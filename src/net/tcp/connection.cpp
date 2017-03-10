// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// #define DEBUG
// #define DEBUG2

#include <gsl/gsl_assert> // Ensures/Expects
#include <net/tcp/connection.hpp>
#include <net/tcp/connection_states.hpp>
#include <net/tcp/packet.hpp>
#include <net/tcp/tcp.hpp>
#include <net/tcp/tcp_errors.hpp>

using namespace net::tcp;
using namespace std;

/*
  This is most likely used in a ACTIVE open
*/
Connection::Connection(TCP& host, port_t local_port, Socket remote, ConnectCallback callback)
  : host_(host),
    local_port_(local_port),
    remote_(remote),
    state_(&Connection::Closed::instance()),
    prev_state_(state_),
    cb{host_.window_size()},
    read_request(),
    writeq(),
    on_connect_{std::move(callback)},
    on_disconnect_({this, &Connection::default_on_disconnect}),
    rtx_timer({this, &Connection::rtx_timeout}),
    timewait_dack_timer({this, &Connection::dack_timeout}),
    queued_(false),
    dack_{0},
    last_ack_sent_{cb.RCV.NXT}
{
  setup_congestion_control();
  debug("<Connection> %s created\n", to_string().c_str());
}

/*
  This is most likely used in a PASSIVE open
*/
/*Connection::Connection(TCP& host, port_t local_port, ConnectCallback cb)
  : Connection(host, local_port, Socket(), std::move(cb))
{
}*/

Connection::TCB::TCB(const uint32_t recvwin)
  : SND{ 0, 0, default_window_size, 0, 0, 0, default_mss, 0, false },
    ISS{(seq_t)4815162342},
    RCV{ 0, recvwin, 0, 0, 0 },
    IRS{0},
    ssthresh{recvwin},
    cwnd{0},
    recover{0},
    TS_recent{0}
{
}

Connection::TCB::TCB()
  : Connection::TCB(default_window_size)
{
}

void Connection::reset_callbacks()
{
  on_disconnect_ = {this, &Connection::default_on_disconnect};
  on_connect_.reset();
  writeq.on_write(nullptr);
  on_error_.reset();
  on_packet_dropped_.reset();
  on_rtx_timeout_.reset();
  on_close_.reset();
  read_request.clean_up();
}

Socket Connection::local() const noexcept {
  return {host_.address(), local_port_};
}

uint16_t Connection::MSDS() const noexcept {
  return std::min(host_.MSS(), cb.SND.MSS) + sizeof(Header);
}

uint16_t Connection::SMSS() const noexcept {
  return host_.MSS();
}

void Connection::read(ReadBuffer&& buffer, ReadCallback callback) {
  try {
    state_->receive(*this, std::forward<ReadBuffer>(buffer));
    read_request.callback = callback;
  }
  catch (const TCPException&) {
    callback(buffer.buffer, buffer.size());
  }
}

size_t Connection::receive(const uint8_t* data, size_t n, bool PUSH) {
  //printf("<Connection::receive> len=%u\n", n);
  // should not be called without an read request
  Ensures(read_request.buffer.capacity());
  Ensures(n);
  auto& buf = read_request.buffer;
  size_t received{0};
  while(n) {
    if (buf.empty()) buf.renew();
    auto read = receive(buf, data+received, n);
    // nothing was read to buffer
    if(!buf.advance(read)) {
      // buffer should be full
      Expects(buf.full());
      // signal the user
      debug2("<Connection::receive> Buffer full - signal user\n");
      if(LIKELY(read_request.callback != nullptr))
        read_request.callback(buf.buffer, buf.size());
      // renew the buffer, releasing the old one
      buf.clear();
    }
    n -= read;
    received += read;
  }
  // n shouldnt be negative
  Expects(n == 0);

  // end of data, signal the user
  if(PUSH) {
    debug2("<Connection::receive> PUSH present - signal user\n");
    if(LIKELY(read_request.callback != nullptr))
      read_request.callback(buf.buffer, buf.size());
    // free buffer
    buf.clear();
  }

  return received;
}


void Connection::write(Chunk buffer)
{
  writeq.push_back(std::move(buffer));
  if(state_->is_writable())
  {
    host_.request_offer(*this);
  }
}

void Connection::offer(size_t& packets)
{
  debug2("<Connection::offer> %s got offered [%u] packets. Usable window is %u.\n",
        to_string().c_str(), packets, usable_window());

  // write until we either cant send more (window closes or no more in queue),
  // or we're out of packets.
  while(can_send() and packets)
  {
    auto packet = create_outgoing_packet();
    packets--;

    size_t written{0};
    size_t x{0};
    // fill the packet with data
    while(can_send() and
      (x = fill_packet(*packet, writeq.nxt_data(), writeq.nxt_rem()) ))
    {
      written += x;
      cb.SND.NXT += x;
      writeq.advance(x);
    }

    packet->set_flag(ACK);

    debug2("<Connection::offer> Wrote %u bytes (%u remaining) with [%u] packets left and a usable window of %u.\n",
           written, buf.remaining, packets, usable_window());

    if(!can_send() or !packets or !writeq.nxt_rem())
      packet->set_flag(PSH);

    if(UNLIKELY(not writeq.has_remaining_requests() and is_closing()))
    {
      debug("<Connection::offer> Setting FIN\n");
      packet->set_flag(FIN);
      cb.SND.NXT++;
    }

    transmit(std::move(packet));
  }

  debug2("<Connection::offer> Finished working offer with [%u] packets left and a queue of (%u) with a usable window of %i\n",
        packets, writeq.size(), usable_window());

  if(can_send() and not queued_)
  {
    host_.queue_offer(shared_from_this());
  }
}

void Connection::writeq_push()
{
  debug2("<Connection::writeq_push> Processing writeq, queued=%u\n", queued_);
  while(not queued_ and can_send())
    host_.request_offer(*this);
}

void Connection::limited_tx() {

  auto packet = create_outgoing_packet();

  debug2("<Connection::limited_tx> UW: %u CW: %u, FS: %u\n", usable_window(), cb.cwnd, flight_size());

  const auto written = fill_packet(*packet, writeq.nxt_data(), writeq.nxt_rem());
  cb.SND.NXT += written;
  packet->set_flag(ACK);

  writeq.advance(written);

  if(UNLIKELY(not writeq.has_remaining_requests() and is_closing()))
  {
    debug("<Connection::limited_tx> Setting FIN\n");
    packet->set_flag(FIN);
    cb.SND.NXT++;
  }

  transmit(std::move(packet));
}

void Connection::writeq_reset() {
  debug2("<Connection::writeq_reset> Reseting.\n");
  writeq.reset();
  rtx_timer.stop();
}

void Connection::open(bool active) {
  try {
    debug("<TCP::Connection::open> Trying to open Connection...\n");
    state_->open(*this, active);
  }
  // No remote host, or state isnt valid for opening.
  catch (const TCPException& e) {
    debug("<TCP::Connection::open> Cannot open Connection. \n");
    signal_error(e);
  }
}

void Connection::close() {
  debug("<TCP::Connection::close> Active close on connection. \n");

  if(is_closing())
    return;

  try {
    state_->close(*this);
    if(is_state(Closed::instance()))
      signal_close();
  }
  catch(const TCPException& err) {
    signal_error(err);
  }
}

void Connection::receive_disconnect() {
  assert(!read_request.buffer.empty());
  auto& buf = read_request.buffer;

  if(LIKELY(read_request.callback != nullptr))
    read_request.callback(buf.buffer, buf.size());
}

void Connection::segment_arrived(Packet_ptr incoming)
{
  //const uint32_t FMASK = (~(0x0000000F | htons(0x08)));
  //uint32_t FMASK = 0xFFFFF7F0;
  //uint32_t FMASK = (((0xf000 | SYN|FIN|RST|URG|ACK) << 16) | 0xffff);
  //printf("pred: %#010x %#010x %#010x %#010x\n",
  //  (((uint32_t*)&incoming->tcp_header())[3]), FMASK, ((((uint32_t*)&incoming->tcp_header())[3]) & FMASK), pred_flags);

  //if( ( (((uint32_t*)&incoming->tcp_header())[3]) & FMASK) == pred_flags)
  //  printf("predicted\n");

  // Let state handle what to do when incoming packet arrives, and modify the outgoing packet.
  switch(state_->handle(*this, std::move(incoming)))
  {
    case State::OK:
      return; // // Do nothing.
    case State::CLOSED:
      debug("<TCP::Connection::receive> (%s => %s) State handle finished with CLOSED. We're done, ask host() to delete the connection.\n",
        prev_state_->to_string().c_str(), state_->to_string().c_str());
      writeq_reset();
      signal_close();
      set_state(Closed::instance());
      break;
  }
}

bool Connection::is_listening() const noexcept {
  return is_state(Listen::instance());
}

__attribute__((weak))
void Connection::deserialize_from(void*) {}
__attribute__((weak))
int  Connection::serialize_to(void*) const {  return 0;  }

Connection::~Connection() {
  // Do all necessary clean up.
  // Free up buffers etc.
  debug2("<Connection::~Connection> Deleted %s\n", to_string().c_str());
  rtx_clear();
}

Packet_ptr Connection::create_outgoing_packet() {
  auto packet = static_unique_ptr_cast<net::tcp::Packet>((host_.inet_).create_packet());
  //auto packet = std::static_pointer_cast<TCP::Packet>(create_packet());

  packet->init();
  // Set Source (local == the current connection)
  packet->set_source(local());
  // Set Destination (remote)
  packet->set_destination(remote_);

  packet->set_win(std::min((cb.RCV.WND >> cb.RCV.wind_shift), (uint32_t)default_window_size));

  if(cb.SND.TS_OK)
    packet->add_tcp_option_aligned<Option::opt_ts_align>(host_.get_ts_value(), cb.TS_recent);
  // Set SEQ and ACK - I think this is OK..
  packet->set_seq(cb.SND.NXT).set_ack(cb.RCV.NXT);
  debug("<TCP::Connection::create_outgoing_packet> Outgoing packet created: %s \n", packet->to_string().c_str());

  return packet;
}

void Connection::transmit(Packet_ptr packet) {
  if(!cb.SND.TS_OK
    and !rttm.active()
    and packet->end() == cb.SND.NXT)
  {
    //printf("<TCP::Connection::transmit> Starting RTT measurement.\n");
    rttm.start(std::chrono::milliseconds{host_.get_ts_value()});
  }
  if(packet->should_rtx() and !rtx_timer.is_running()) {
    rtx_start();
  }
  if(packet->isset(ACK))
    last_ack_sent_ = cb.RCV.NXT;

  //if(packet->has_tcp_data()) printf("<Connection::transmit> TX %s - NXT:%u\n", packet->to_string().c_str(), cb.SND.NXT);

  host_.transmit(std::move(packet));
}

bool Connection::handle_ack(const Packet& in)
{
  //printf("<Connection> RX ACK: %s\n", in.to_string().c_str());

  // Calculate true window due to WS option
  const uint32_t true_win = in.win() << cb.SND.wind_shift;
  /*
    (a) the receiver of the ACK has outstanding data
    (b) the incoming acknowledgment carries no data
    (c) the SYN and FIN bits are both off
    (d) the acknowledgment number is equal to the greatest acknowledgment
    received on the given connection (TCP.UNA from [RFC793]) and
    (e) the advertised window in the incoming acknowledgment equals the
    advertised window in the last incoming acknowledgment.
  */
  // Duplicate ACK
  // Needs to be checked before (SEG.ACK >= SND.UNA)
  if(UNLIKELY(is_dup_ack(in, true_win)))
  {
    dup_acks_++;
    on_dup_ack(in);
    return false;
  } // < dup ack

  // new ack
  else if(LIKELY(in.ack() >= cb.SND.UNA))
  {
    if(is_win_update(in, true_win))
    {
      cb.SND.WND = true_win;
      cb.SND.WL1 = in.seq();
      cb.SND.WL2 = in.ack();
      //printf("<Connection::handle_ack> Window update (%u)\n", cb.SND.WND);
    }
    //pred_flags = htonl((in.tcp_header_length() << 26) | 0x10 | cb.SND.WND >> cb.SND.wind_shift);

    // [RFC 6582] p. 8
    prev_highest_ack_ = cb.SND.UNA;
    highest_ack_ = in.ack();

    if(cb.SND.TS_OK)
    {
      auto* ts = parse_ts_option(in);
      last_acked_ts_ = ts->ecr;
    }

    cb.SND.UNA = in.ack();

    rtx_ack(in.ack());

    take_rtt_measure(in);

    // do either congctrl or fastrecov according to New Reno
    (not fast_recovery_)
      ? congestion_control(in) : fast_recovery(in);

    dup_acks_ = 0;

    if(in.has_tcp_data() or in.isset(FIN))
      return true;

  } // < new ack

  // ACK outside
  else {
    return true;
  }
  return false;
}

void Connection::congestion_control(const Packet& in)
{
  const size_t bytes_acked = highest_ack_ - prev_highest_ack_;

  // update recover
  cb.recover = cb.SND.NXT;

  // slow start
  if(cb.slow_start())
  {
    reno_increase_cwnd(bytes_acked);
    debug2("<Connection::handle_ack> Slow start. cwnd=%u uw=%u\n",
      cb.cwnd, usable_window());
  }
  // congestion avoidance
  else
  {
    // increase cwnd once per RTT
    cb.cwnd += std::max(SMSS()*SMSS()/cb.cwnd, (uint32_t)1);
    debug2("<Connection::handle_ack> Congestion avoidance. cwnd=%u uw=%u\n",
      cb.cwnd, usable_window());
  } // < congestion avoidance

  // try to write
  if(can_send() and !in.has_tcp_data())
  {
    debug2("<Connection::handle_ack> Can send UW: %u SMSS: %u\n", usable_window(), SMSS());
    send_much();
  }
}

void Connection::fast_recovery(const Packet& in)
{
  // partial ack
  /*
    Partial acknowledgments:
    If this ACK does *not* acknowledge all of the data up to and
    including recover, then this is a partial ACK.
  */
  if(in.ack() < cb.recover)
  {
    const size_t bytes_acked = highest_ack_ - prev_highest_ack_;
    debug2("<Connection::handle_ack> Partial ACK - recover: %u NXT: %u ACK: %u\n", cb.recover, cb.SND.NXT, in.ack());
    reno_deflate_cwnd(bytes_acked);
    // RFC 4015
    /*
    If the value of the Timestamp Echo Reply field of the
    acceptable ACK's Timestamps option is smaller than the
    value of RetransmitTS, then proceed to step (5),

    If the acceptable ACK carries a DSACK option [RFC2883],
    then proceed to step (DONE),

    else if during the lifetime of the TCP connection the TCP
    sender has previously received an ACK with a DSACK option,
    or the acceptable ACK does not acknowledge all outstanding
    data, then proceed to step (6),
    */
    //if(cb.SND.TS_OK and ntohl(last_acked_ts_) < rtx_ts_)
    //  spurious_recovery = (rtx_attempt_ > 0) ? SPUR_TO : LATE_SPUR_TO;
    /*
     (8) Resume the transmission with previously unsent data:
        Set SND.NXT <- SND.MAX
    */
    //if(spurious_recovery == SPUR_TO)
    //  limited_tx();
    /*
      Reverse the congestion control state:
      If the acceptable ACK has the ECN-Echo flag [RFC3168] set,
      then
         proceed to step (DONE);
      else set
         cwnd <- FlightSize + min (bytes_acked, IW)
         ssthresh <- pipe_prev
     Proceed to step (DONE).
    */
    //else if(spurious_recovery == LATE_SPUR_TO)
    //  cb.cwnd = flight_size() + std::min(bytes_acked, 3*(uint32_t)SMSS());

    if(!reno_fpack_seen) {
      rtx_reset();
      reno_fpack_seen = true;
    }

    retransmit();

    // send one segment if possible
    if(can_send())
      limited_tx();
  } // < partial ack

  // full ack
  /*
    Full acknowledgments:
    If this ACK acknowledges all of the data up to and including
    recover, then the ACK acknowledges all the intermediate segments
    sent between the original transmission of the lost segment and
    the receipt of the third duplicate ACK.
  */
  else
  {
    debug2("<Connection::handle_ack> Full ACK.\n");
    finish_fast_recovery();
    writeq_push();
  } // < full ack
}

/*
  Reno [RFC 5681] p. 9

  What to do when received ACK is a duplicate.
*/
void Connection::on_dup_ack(const Packet& in)
{
  // if less than 3 dup acks
  if(dup_acks_ < 3)
  {
    /*
      TCP SHOULD send a segment of previously unsent data per [RFC3042]
      provided that the receiver's advertised window allows, the total
      FlightSize would remain less than or equal to cwnd plus 2*SMSS,
      and that new data is available for transmission
    */
    if(limited_tx_
      and cb.SND.WND >= SMSS()
      and (flight_size() <= cb.cwnd + 2*SMSS())
      and writeq.has_remaining_requests())
    {
      limited_tx(); // send one segment
    }
  }

  // 3 dup acks
  else if(dup_acks_ == 3)
  {
    debug("<TCP::Connection::on_dup_ack> Dup ACK == 3 - %u\n", cb.SND.UNA);

    if(cb.SND.UNA - 1 > cb.recover)
      goto fast_rtx;

    // 4.2.  Timestamp Heuristic
    if(cb.SND.TS_OK)
    {
      auto* ts = parse_ts_option(in);
      if(ts != nullptr and last_acked_ts_ == ts->ecr)
      {
        goto fast_rtx;
      }
    }
    // 4.1.  ACK Heuristic
    else if(send_window() > SMSS() and (highest_ack_ - prev_highest_ack_ <= 4*SMSS()))
    {
      goto fast_rtx;
    }

    return;

    fast_rtx:
    {
      cb.recover = cb.SND.NXT;
      debug("<TCP::Connection::on_dup_ack> Enter Recovery - Flight Size: %u\n", flight_size());
      fast_retransmit();
    }
  }

  // > 3 dup acks
  else {
    cb.cwnd += SMSS();
    // send one segment if possible
    //if(can_send())
    //  limited_tx();
  }
}

/*
  [RFC 6298]

   (5.2) When all outstanding data has been acknowledged, turn off the
         retransmission timer.

   (5.3) When an ACK is received that acknowledges new data, restart the
         retransmission timer so that it will expire after RTO seconds
         (for the current value of RTO).
*/
void Connection::rtx_ack(const seq_t ack) {
  const auto acked = ack - prev_highest_ack_;
  // what if ack is from handshake / fin?
  writeq.acknowledge(acked);
  /*
    When all outstanding data has been acknowledged, turn off the
    retransmission timer.
  */
  if(cb.SND.UNA == cb.SND.NXT) {
    rtx_stop();
    rtx_attempt_ = 0;
  }
  /*
    When an ACK is received that acknowledges new data, restart the
    retransmission timer so that it will expire after RTO seconds
    (for the current value of RTO).
  */
  else if(acked > 0) {
    rtx_reset();
    rtx_attempt_ = 0;
  }

  //printf("<TCP::Connection::rt_acknowledge> ACK'ed %u packets. rtx_q: %u\n",
  //  x-rtx_q.size(), rtx_q.size());
}

void Connection::take_rtt_measure(const Packet& packet)
{
  if(cb.SND.TS_OK)
  {
    auto* ts = parse_ts_option(packet);
    rttm.rtt_measurement(RTTM::milliseconds{host_.get_ts_value() - ntohl(ts->ecr)});
  }
  else if(rttm.active())
  {
    rttm.stop(RTTM::milliseconds{host_.get_ts_value()});
  }
}

/*
  Assumption
  Retransmission will only occur when one of the following are true:
  * There is data to be sent  (!SYN-SENT || !SYN-RCV)
  * Last packet had SYN       (SYN-SENT || SYN-RCV)
  * Last packet had FIN       (FIN-WAIT-1 || LAST-ACK)

*/
void Connection::retransmit() {
  auto packet = create_outgoing_packet();
  // If not retransmission of a pure SYN packet, add ACK
  if(!is_state(SynSent::instance())) {
    packet->set_flag(ACK);
  }
  // If retransmission from either SYN-SENT or SYN-RCV, add SYN
  if(UNLIKELY(is_state(SynSent::instance()) or is_state(SynReceived::instance())))
  {
    packet->set_flag(SYN);
    packet->set_seq(cb.SND.UNA);
    syn_rtx_++;
  }
  // If not, check if there is data and retransmit
  else if(writeq.size()) {
    auto& buf = writeq.una();
    debug2("<Connection::retransmit> With data (wq.sz=%u) buf.unacked=%u\n",
      writeq.size(), buf.length() - buf.acknowledged);
    fill_packet(*packet, buf.data() + writeq.acked(), buf.length() - writeq.acked());
  }

  packet->set_seq(cb.SND.UNA);

  /*
  Set a "RetransmitTS" variable to the value of the
  Timestamp Value field of the Timestamps option included in
  the retransmit sent when loss recovery is initiated.  A
  TCP sender must ensure that RetransmitTS does not get
  overwritten as loss recovery progresses, e.g., in case of
  a second timeout and subsequent second retransmit of the
  same octet.
  */
  /*if(cb.SND.TS_OK and !fast_recovery_)
  {
    auto* ts = parse_ts_option(*packet);
    rtx_ts_ = ts->get_val();
    spurious_recovery = 0;
  }*/

  // If retransmission of a FIN packet
  // TODO: find a solution to this
  //if((is_state(FinWait1::instance()) or is_state(LastAck::instance()))
  //  and !writeq.has_remaining_requests())
  //{
  //  packet->set_flag(FIN);
  //}

  //printf("<TCP::Connection::retransmit> rseq=%u \n", packet->seq() - cb.ISS);

  /*
    Every time a packet containing data is sent (including a
    retransmission), if the timer is not running, start it running
    so that it will expire after RTO seconds (for the current value
    of RTO).
  */
  if(packet->should_rtx() and !rtx_timer.is_running()) {
    rtx_start();
  }
  debug("<Connection::retransmit> RTX: %s\n", packet->to_string().c_str());
  host_.transmit(std::move(packet));
}

void Connection::rtx_clear() {
  if(rtx_timer.is_running()) {
    rtx_stop();
    debug2("<Connection::rtx_clear> Rtx cleared\n");
  }
}

/*
  When the retransmission timer expires, do the following:

 (5.4) Retransmit the earliest segment that has not been acknowledged
       by the TCP receiver.

 (5.5) The host MUST set RTO <- RTO * 2 ("back off the timer").  The
       maximum value discussed in (2.5) above may be used to provide
       an upper bound to this doubling operation.

 (5.6) Start the retransmission timer, such that it expires after RTO
       seconds (for the value of RTO after the doubling operation
       outlined in 5.5).

 (5.7) If the timer expires awaiting the ACK of a SYN segment and the
       TCP implementation is using an RTO less than 3 seconds, the RTO
       MUST be re-initialized to 3 seconds when data transmission
       begins (i.e., after the three-way handshake completes).
*/
void Connection::rtx_timeout() {
  debug("<Connection::RTX@timeout> Timed out (RTO %lld ms). FS: %u\n",
    rttm.rto_ms().count(), flight_size());

  signal_rtx_timeout();
  // experimental
  if(rto_limit_reached()) {
    debug("<TCP::Connection::rtx_timeout> RTX attempt limit reached, closing.\n");
    abort();
    return;
  }

  // retransmit SND.UNA
  retransmit();

  if(cb.SND.UNA != cb.ISS) {
    // "back off" timer
    rttm.RTO *= 2.0;
  }
  // we never queue SYN packets since they don't carry data..
  else {
    rttm.RTO = std::chrono::seconds(3);
  }
  // timer need to be restarted
  rtx_start();

  /*
    [RFC 5681] p. 7

    When a TCP sender detects segment loss using the retransmission timer
    and the given segment has not yet been resent by way of the
    retransmission timer, the value of ssthresh MUST be set to no more
    than the value given in equation (4):

      ssthresh = max (FlightSize / 2, 2*SMSS)
  */
  if(rtx_attempt_++ == 0)
  {
    // RFC 4015
    /*
      Before the variables cwnd and ssthresh get updated when
      loss recovery is initiated:
          pipe_prev <- max (FlightSize, ssthresh)
          SRTT_prev <- SRTT + (2 * G)
          RTTVAR_prev <- RTTVAR
    */
    //pipe_prev   = std::max(flight_size(), cb.ssthresh);
    //SRTT_prev   = RTTM::seconds{rttm.SRTT.count() + (2 * RTTM::CLOCK_G)};
    //RTTVAR_prev = rttm.RTTVAR;
    reduce_ssthresh();
  }

  /*
    [RFC 6582] p. 6
    After a retransmit timeout, record the highest sequence number
    transmitted in the variable recover, and exit the fast recovery
    procedure if applicable.
  */

  // update recover
  cb.recover = cb.SND.NXT;

  if(fast_recovery_) // not sure if this is correct
    finish_fast_recovery();

  //cb.cwnd = SMSS();
  reno_init_cwnd(3); // experimental
  /*
    NOTE: It's unclear which one comes first, or if finish_fast_recovery includes changing the cwnd.
  */
}

seq_t Connection::generate_iss() {
  return TCP::generate_iss();
}

void Connection::set_state(State& state) {
  prev_state_ = state_;
  state_ = &state;
  debug("<TCP::Connection::set_state> %s => %s \n",
        prev_state_->to_string().c_str(), state_->to_string().c_str());
}

void Connection::timewait_start() {
  const auto timeout = 2 * host().MSL(); // 60 seconds
  timewait_dack_timer.restart(timeout, {this, &Connection::timewait_timeout});
}

void Connection::timewait_restart() {
  const auto timeout = 2 * host().MSL(); // 60 seconds
  timewait_dack_timer.restart(timeout);
}

void Connection::send_ack() {
  auto packet = outgoing_packet();
  packet->set_flag(ACK);
  transmit(std::move(packet));
}

bool Connection::use_dack() const noexcept {
  return host_.DACK_timeout() > std::chrono::milliseconds::zero();
}

void Connection::start_dack()
{
  Ensures(use_dack());
  ++dack_;
  timewait_dack_timer.start(host_.DACK_timeout());
}

void Connection::signal_close() {
  debug("<Connection::signal_close> It's time to delete this connection. \n");

  // call user callback
  if(on_close_) on_close_();

  // clean up all copies, delegates and timers
  clean_up();
}

void Connection::clean_up() {
  // clear timers if active
  rtx_clear();
  if(timewait_dack_timer.is_running())
    timewait_dack_timer.stop();

  // necessary to keep the shared_ptr alive during the whole function after _on_cleanup_ is called
  // avoids connection being destructed before function is done
  auto shared = shared_from_this();
  // clean up all other copies
  // either in TCP::listeners_ (open) or Listener::syn_queue_ (half-open)
  if(_on_cleanup_) _on_cleanup_(shared);

  on_connect_.reset();
  on_disconnect_.reset();
  on_error_.reset();
  on_packet_dropped_.reset();
  on_rtx_timeout_.reset();
  on_close_.reset();
  read_request.clean_up();
  _on_cleanup_.reset();

  debug("<Connection::clean_up> Succesfully cleaned up %s\n", to_string().c_str());
}

std::string Connection::TCB::to_string() const {
  ostringstream os;
  os << "SND"
     << " .UNA = " << SND.UNA
     << " .NXT = " << SND.NXT
     << " .WND = " << SND.WND
     << " .UP = " << SND.UP
     << " .WL1 = " << SND.WL1
     << " .WL2 = " << SND.WL2
     << " ISS = " << ISS
     << "\n RCV"
     << " .NXT = " << RCV.NXT
     << " .WND = " << RCV.WND
     << " .UP = " << RCV.UP
     << " IRS = " << IRS;
  return os.str();
}

void Connection::parse_options(const Packet& packet) {
  assert(packet.has_tcp_options());
  debug("<TCP::parse_options> Parsing options. Offset: %u, Options: %u \n",
        packet.offset(), packet.tcp_options_length());

  auto* opt = packet.tcp_options();

  while((Byte*)opt < packet.tcp_data()) {

    auto* option = (Option*)opt;

    switch(option->kind) {

    case Option::END: {
      return;
    }

    case Option::NOP: {
      opt++;
      break;
    }

    case Option::MSS: {

      if(UNLIKELY(option->length != sizeof(Option::opt_mss)))
        throw TCPBadOptionException{Option::MSS, "length != 4"};

      if(UNLIKELY(!packet.isset(SYN)))
        throw TCPBadOptionException{Option::MSS, "Non-SYN packet"};

      auto* opt_mss = (Option::opt_mss*)option;
      cb.SND.MSS = ntohs(opt_mss->mss);

      debug2("<TCP::parse_options@Option:MSS> MSS: %u \n", cb.SND.MSS);

      opt += option->length;
      break;
    }

    case Option::WS: {

      if(UNLIKELY(option->length != sizeof(Option::opt_ws)))
        throw TCPBadOptionException{Option::WS, "length != 3"};

      if(UNLIKELY(!packet.isset(SYN)))
        throw TCPBadOptionException{Option::WS, "Non-SYN packet"};

      if(host_.uses_wscale())
      {
        const auto& opt_ws = (Option::opt_ws&)*option;
        cb.SND.wind_shift = std::min(opt_ws.shift_cnt, (uint8_t)14);
        cb.RCV.wind_shift = host_.wscale();

        debug2("<Connection::parse_options@WS> WS: %u Calc: %u\n",
          cb.SND.wind_shift, cb.SND.WND << cb.SND.wind_shift);
      }

      opt += option->length;
      break;
    }

    case Option::TS: {

      if(host_.uses_timestamps())
      {
        const auto& opt_ts = (Option::opt_ts&)*option;

        if(UNLIKELY(packet.isset(SYN)))
        {
          cb.SND.TS_OK = true;
          cb.TS_recent = ntohl(opt_ts.val);
        }
        else if(ntohl(opt_ts.val) >= cb.TS_recent and packet.seq() <= last_ack_sent_)
        {
          cb.TS_recent = ntohl(opt_ts.val);
        }
      }
      opt += option->length;
    }

    default:
      return;
    }
  }
}

void Connection::add_option(Option::Kind kind, Packet& packet) {

  switch(kind) {

  case Option::MSS: {
    packet.add_tcp_option<Option::opt_mss>(host_.MSS());
    debug2("<TCP::Connection::add_option@Option::MSS> Packet: %s - MSS: %u\n",
           packet.to_string().c_str(), ntohs(*(uint16_t*)(packet.tcp_options()+2)));
    break;
  }

  case Option::WS: {
    packet.add_tcp_option<Option::opt_ws>(host_.wscale());
    break;
  }

  case Option::TS: {
    const uint32_t ts_ecr = (packet.isset(ACK)) ? cb.TS_recent : 0;
    packet.add_tcp_option<Option::opt_ts>(host_.get_ts_value(), ts_ecr);
    break;
  }
  default:
    break;
  }
}

Option::opt_ts* Connection::parse_ts_option(const Packet& packet) const
{
  auto* opt = packet.tcp_options();

  while(((Option*)opt)->kind == Option::NOP and opt < (uint8_t*)packet.tcp_data())
    opt++;

  return (((Option*)opt)->kind == Option::TS) ? (Option::opt_ts*)opt : nullptr;
}

bool Connection::uses_window_scaling() const noexcept
{
  return host_.uses_wscale();
}

bool Connection::uses_timestamps() const noexcept
{
  return host_.uses_timestamps();
}

void Connection::drop(const Packet& packet, Drop_reason reason)
{
  signal_packet_dropped(packet, reason);
  host_.drop(packet);
}

void Connection::default_on_disconnect(Connection_ptr conn, Disconnect) {
  if(!conn->is_closing())
    conn->close();
}

void Connection::reduce_ssthresh() {
  auto fs = flight_size();

  const uint32_t two_seg = 2*SMSS();

  if(limited_tx_)
    fs = (fs >= two_seg) ? fs - two_seg : 0;

  cb.ssthresh = std::max( (fs / 2), two_seg );
  debug2("<TCP::Connection::reduce_ssthresh> Slow start threshold reduced: %u\n",
    cb.ssthresh);
}

void Connection::fast_retransmit() {
  debug("<TCP::Connection::fast_retransmit> Fast retransmit initiated.\n");
  // reduce sshtresh
  reduce_ssthresh();
  // retransmit segment starting SND.UNA
  retransmit();
  // inflate congestion window with the 3 packets we got dup ack on.
  cb.cwnd = cb.ssthresh + 3*SMSS();
  fast_recovery_ = true;
}

void Connection::finish_fast_recovery() {
  reno_fpack_seen = false;
  fast_recovery_ = false;
  //cb.cwnd = std::min(cb.ssthresh, std::max(flight_size(), (uint32_t)SMSS()) + SMSS());
  cb.cwnd = cb.ssthresh;
  debug("<TCP::Connection::finish_fast_recovery> Finished Fast Recovery - Cwnd: %u\n", cb.cwnd);
}
