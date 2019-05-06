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

#include <common> // Ensures/Expects
#include <net/tcp/connection.hpp>
#include <net/tcp/connection_states.hpp>
#include <net/tcp/tcp.hpp>
#include <net/tcp/tcp_errors.hpp>

using namespace net::tcp;
using namespace std;

Connection::Connection(TCP& host, Socket local, Socket remote, ConnectCallback callback)
  : host_(host),
    local_{std::move(local)}, remote_{std::move(remote)},
    is_ipv6_(local_.address().is_v6()),
    state_(&Connection::Closed::instance()),
    prev_state_(state_),
    cb{(is_ipv6_) ? default_mss_v6 : default_mss, host_.window_size()},
    read_request(nullptr),
    writeq(),
    on_connect_{std::move(callback)},
    on_disconnect_({this, &Connection::default_on_disconnect}),
    rtx_timer({this, &Connection::rtx_timeout}),
    timewait_dack_timer({this, &Connection::dack_timeout}),
    recv_wnd_getter{nullptr},
    queued_(false),
    dack_{0},
    last_ack_sent_{cb.RCV.NXT},
    smss_{MSS()}
{
  setup_congestion_control();
  //printf("<Connection> Created %p %s  ACTIVE: %u\n", this,
  //        to_string().c_str(), host_.active_connections());
}

Connection::~Connection()
{
  //printf("<Connection> Deleted %p %s  ACTIVE: %zu\n", this,
  //        to_string().c_str(), host_.active_connections());

  rtx_clear();
}

void Connection::_on_read(size_t recv_bufsz, ReadCallback cb)
{
  (void) recv_bufsz;
  if(read_request == nullptr)
  {
    Expects(bufalloc != nullptr);
    read_request.reset(
      new Read_request(this->cb.RCV.NXT, host_.min_bufsize(), host_.max_bufsize(), bufalloc.get()));
    read_request->on_read_callback = cb;
    const size_t avail_thres = host_.max_bufsize() * Read_request::buffer_limit;
    bufalloc->on_avail(avail_thres, {this, &Connection::trigger_window_update});
  }
  // read request is already set, only reset if new size.
  else
  {
    //printf("on_read already set\n");
    read_request->on_read_callback = cb;
    // this will flush the current data to the user (if any)
    read_request->reset(this->cb.RCV.NXT);

    // due to throwing away buffers (and all data) we also
    // need to clear the sack list if anything is stored here.
    if(sack_list)
      sack_list->clear();
  }
}

void Connection::_on_data(DataCallback cb) {
  if(read_request == nullptr)
  {
    Expects(bufalloc != nullptr);
    read_request.reset(
      new Read_request(this->cb.RCV.NXT, host_.min_bufsize(), host_.max_bufsize(), bufalloc.get()));
    read_request->on_data_callback = cb;
    const size_t avail_thres = host_.max_bufsize() * Read_request::buffer_limit;
    bufalloc->on_avail(avail_thres, {this, &Connection::trigger_window_update});
  }
  // read request is already set, only reset if new size.
  else
  {
    //printf("on_read already set\n");
    read_request->on_data_callback = cb;

    read_request->reset(this->cb.RCV.NXT);

    // due to throwing away buffers (and all data) we also
    // need to clear the sack list if anything is stored here.
    if(sack_list)
      sack_list->clear();
  }
}


Connection_ptr Connection::retrieve_shared() {
  return host_.retrieve_shared(this);
}

/*
  This is most likely used in a PASSIVE open
*/
/*Connection::Connection(TCP& host, port_t local_port, ConnectCallback cb)
  : Connection(host, local_port, Socket(), std::move(cb))
{
}*/

Connection::TCB::TCB(const uint16_t mss, const uint32_t recvwin)
  : SND{ 0, 0, default_window_size, 0, 0, 0, mss, 0, false },
    ISS{(seq_t)4815162342},
    RCV{ 0, recvwin, 0, 0, 0 },
    IRS{0},
    ssthresh{recvwin},
    cwnd{0},
    recover{0},
    TS_recent{0}
{
}

Connection::TCB::TCB(const uint16_t mss)
  : Connection::TCB(mss, default_window_size)
{
}

void Connection::reset_callbacks()
{
  on_disconnect_ = {this, &Connection::default_on_disconnect};
  on_connect_.reset();
  writeq.on_write(nullptr);
  on_close_.reset();
  recv_wnd_getter.reset();
  if(read_request) {
    read_request->on_read_callback.reset();
    read_request->on_data_callback.reset();
  }
}

uint16_t Connection::MSS() const noexcept {
  return host_.MSS(ipv());
}

uint16_t Connection::MSDS() const noexcept {
  return std::min(MSS(), cb.SND.MSS) + sizeof(Header);
}

size_t Connection::receive(seq_t seq, const uint8_t* data, size_t n, bool PUSH) {
  // should not be called without an read request
  Expects(read_request);
  Expects(n > 0);
  size_t received{0};

  while(n)
  {
    auto read = read_request->insert(seq, data + received, n, PUSH);

    n -= read; // subtract amount of data left to insert
    received += read; // add to the total recieved
    seq += read; // advance the sequence number
  }

  return received;
}


void Connection::write(buffer_t buffer)
{
  if (UNLIKELY(buffer->size() == 0)) {
    throw TCP_error("Can't write zero bytes to TCP stream");
  }

  // Only write if allowed
  if(state_->is_writable())
  {
    // add to queue
    writeq.push_back(std::move(buffer));

    // request packets if connected, else let ACK clock do the writing
    if(state_->is_connected())
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

  if (this->can_send() and not this->is_queued())
  {
    host_.queue_offer(*this);
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

void Connection::open(bool active)
{
  debug("<TCP::Connection::open> Trying to open Connection...\n");
  state_->open(*this, active);
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
  catch(const TCPException&) {
    // just ignore for now, it's kinda stupid its even throwing (i think)
    // early return is_closing will probably prevent this from happening
  }
}

void Connection::receive_disconnect() {
  Expects(read_request and read_request->size());

  if(read_request->on_read_callback) {
    // TODO: consider adding back when SACK is complete
    //auto& buf = read_request->buffer;
    //if (buf.size() > 0 && buf.missing() == 0)
    //    read_request->callback(buf.buffer(), buf.size());
  }
}

void Connection::update_fin(const Packet_view& pkt)
{
  Expects(pkt.isset(FIN));
  // should be no problem calling multiple times
  // as long as fin do not change (which it absolutely shouldnt)
  fin_recv_ = true;
  fin_seq_  = pkt.end();
}

void Connection::handle_fin()
{
  // Advance RCV.NXT over the FIN
  cb.RCV.NXT++;
  const auto snd_nxt = cb.SND.NXT;

  // empty the read buffer
  //if(read_request and read_request->size())
  //  receive_disconnect();

  // signal disconnect to the user
  signal_disconnect(Disconnect::CLOSING);

  // only ack FIN if user callback didn't result in a sent packet
  if(cb.SND.NXT == snd_nxt) {
    debug2("<Connection::handle_fin> acking FIN\n");
    auto packet = outgoing_packet();
    packet->set_ack(cb.RCV.NXT).set_flag(ACK);
    transmit(std::move(packet));
  }
}

void Connection::segment_arrived(Packet_view& incoming)
{
  //const uint32_t FMASK = (~(0x0000000F | htons(0x08)));
  //uint32_t FMASK = 0xFFFFF7F0;
  //uint32_t FMASK = (((0xf000 | SYN|FIN|RST|URG|ACK) << 16) | 0xffff);
  //printf("pred: %#010x %#010x %#010x %#010x\n",
  //  (((uint32_t*)&incoming->tcp_header())[3]), FMASK, ((((uint32_t*)&incoming->tcp_header())[3]) & FMASK), pred_flags);

  //if( ( (((uint32_t*)&incoming->tcp_header())[3]) & FMASK) == pred_flags)
  //  printf("predicted\n");

  // Let state handle what to do when incoming packet arrives, and modify the outgoing packet.
  switch(state_->handle(*this, incoming))
  {
    case State::OK:
      return; // // Do nothing.
    case State::CLOSED:
      debug("<TCP::Connection::receive> (%s => %s) State handle finished with CLOSED. We're done, ask host() to delete the connection.\n",
        prev_state_->to_string().c_str(), state_->to_string().c_str());
      writeq_reset();
      signal_close();
      /// NOTE: connection is dead here!
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

Packet_view_ptr Connection::create_outgoing_packet()
{
  update_rcv_wnd();
  auto packet = (is_ipv6_) ?
    host_.create_outgoing_packet6() : host_.create_outgoing_packet();
  // Set Source (local == the current connection)
  packet->set_source(local_);
  // Set Destination (remote)
  packet->set_destination(remote_);

  packet->set_win(std::min((cb.RCV.WND >> cb.RCV.wind_shift), (uint32_t)default_window_size));

  if(cb.SND.TS_OK)
    packet->add_tcp_option_aligned<Option::opt_ts_align>(host_.get_ts_value(), cb.get_ts_recent());

  // Add SACK option (if any entries)
  if(UNLIKELY(sack_list and sack_list->size()))
  {
    Expects(sack_perm);

    auto entries = sack_list->recent_entries();
    // swap to network endian before adding to packet
    for(auto& ent : entries)
      ent.swap_endian();

    packet->add_tcp_option_aligned<Option::opt_sack_align>(entries);
  }

  // Set SEQ and ACK
  packet->set_seq(cb.SND.NXT).set_ack(cb.RCV.NXT);
  debug("<TCP::Connection::create_outgoing_packet> Outgoing packet created: %s \n", packet->to_string().c_str());

  return packet;
}

void Connection::transmit(Packet_view_ptr packet) {
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

  //printf("<Connection::transmit> TX %s\n%s\n", packet->to_string().c_str(), to_string().c_str());

  host_.transmit(std::move(packet));
}

bool Connection::handle_ack(const Packet_view& in)
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

  if(is_win_update(in, true_win))
  {
    //if(cb.SND.WND < SMSS()*2)
    //  printf("Win update: %u => %u\n", cb.SND.WND, true_win);
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
    const auto* ts = in.ts_option();
    // reparse to avoid case when stored ts suddenly get lost
    if(ts == nullptr)
      ts = in.parse_ts_option();

    if(ts != nullptr) // TODO: not sure the packet is valid if TS missing
      last_acked_ts_ = ts->ecr;
  }

  cb.SND.UNA = in.ack();

  rtx_ack(in.ack());

  update_rcv_wnd();

  take_rtt_measure(in);

  // do either congctrl or fastrecov according to New Reno
  (not fast_recovery_)
    ? congestion_control(in) : fast_recovery(in);

  dup_acks_ = 0;

  if(in.has_tcp_data() or in.isset(FIN))
    return true;

  // < new ack
  // Nothing to process
  return false;
}

void Connection::congestion_control(const Packet_view& in)
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
  if(can_send() and (!in.has_tcp_data() or cb.RCV.WND < in.tcp_data_length()))
  {
    debug2("<Connection::handle_ack> Can send UW: %u SMSS: %u\n", usable_window(), SMSS());
    send_much();
  }
}

void Connection::fast_recovery(const Packet_view& in)
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
void Connection::on_dup_ack(const Packet_view& in)
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
    //printf("<TCP::Connection::on_dup_ack> Dup ACK == 3 - UNA=%u recover=%u\n", cb.SND.UNA, cb.recover);

    if(cb.SND.UNA - 1 > cb.recover)
      goto fast_rtx;

    // 4.2.  Timestamp Heuristic
    if(cb.SND.TS_OK)
    {
      const auto* ts = in.ts_option();
      // reparse to avoid case when stored ts suddenly get lost
      if(ts == nullptr)
        ts = in.parse_ts_option();

      if(ts != nullptr)
      {
        if(last_acked_ts_ == ts->ecr)
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
      debug("<TCP::Connection::on_dup_ack> Enter Recovery %u - Flight Size: %u\n",
        cb.recover, flight_size());
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

void Connection::trigger_window_update(os::mem::Pmr_resource& res)
{
  const auto reserve = (host_.max_bufsize() * Read_request::buffer_limit);
  if(res.allocatable() >= reserve and cb.RCV.WND == 0) {
    //printf("allocatable=%zu cur_win=%u\n", res.allocatable(), cb.RCV.WND);
    send_window_update();
  }
}

uint32_t Connection::calculate_rcv_wnd() const
{
  // PRECISE REPORTING
  if(UNLIKELY(read_request == nullptr))
    return 0xffff;

  const auto& rbuf = read_request->front();
  auto remaining = rbuf.capacity() - rbuf.size();

  auto buf_avail = bufalloc->allocatable() + remaining;
  auto reserve   = (host_.max_bufsize() * Read_request::buffer_limit);
  auto win = buf_avail > reserve ? buf_avail - reserve : 0;

  return (win < SMSS()) ? 0 : win; // Avoid small silly windows

  // REPORT CHUNKWISE
  /*
  //auto allocatable = bufalloc->allocatable();
  const auto& rbuf = read_request->front();

  auto win = cb.RCV.WND;
  if (bufalloc->allocatable() < rbuf.capacity()) {
    printf("[connection] Allocatable data is less than capacity. Win 0. \n");
    win = 0;
  } else {
    win = bufalloc->allocatable() - rbuf.capacity();
  }

  return win;
  */

  // REPORT CHUNKWISE FROM ALLOCATOR
  //return bufalloc->allocatable();
}

/*
  7. Process the segment text

  If a packet has data, process the data.

  [RFC 793] Page 74:

  Once in the ESTABLISHED state, it is possible to deliver segment
  text to user RECEIVE buffers.  Text from segments can be moved
  into buffers until either the buffer is full or the segment is
  empty.  If the segment empties and carries an PUSH flag, then
  the user is informed, when the buffer is returned, that a PUSH
  has been received.

  When the TCP takes responsibility for delivering the data to the
  user it must also acknowledge the receipt of the data.

  Once the TCP takes responsibility for the data it advances
  RCV.NXT over the data accepted, and adjusts RCV.WND as
  apporopriate to the current buffer availability.  The total of
  RCV.NXT and RCV.WND should not be reduced.

  Please note the window management suggestions in section 3.7.

  Send an acknowledgment of the form:

  <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK>

  This acknowledgment should be piggybacked on a segment being
  transmitted if possible without incurring undue delay.
*/
void Connection::recv_data(const Packet_view& in)
{
  Expects(in.has_tcp_data());

  // just drop the packet if we don't have a recv wnd / buffer available.
  // this shouldn't be necessary with well behaved connections.
  // I also think we shouldn't reach this point due to State::check_seq checking
  // if we're inside the window. if packet is out of order tho we can change the RCV wnd (i think).
  /*if(UNLIKELY(bufalloc->allocatable() < host_.max_bufsize())) {
    drop(in, Drop_reason::RCV_WND_ZERO);
    return;
  }*/

  size_t length = in.tcp_data_length();

  if(UNLIKELY(cb.RCV.WND < length))
  {
    drop(in, Drop_reason::RCV_WND_ZERO);
    update_rcv_wnd();
    send_ack();
    return;
   }

  // Keep track if a packet is being sent during the async read callback
  const auto snd_nxt = cb.SND.NXT;

  // The packet we expect
  if(cb.RCV.NXT == in.seq())
  {

    // If we had packet loss before (and SACK is on)
    // we need to clear up among the blocks
    // and increase the total amount of bytes acked
    if(UNLIKELY(sack_list != nullptr))
    {
      const auto res = sack_list->new_valid_ack(in.seq(), length);
      // if any bytes are cleared up in sack, increase expected sequence number
      cb.RCV.NXT += res.blocksize;

      // if the ACK was a new ack, but some of the latter part of the segment
      // was already received by SACK, it will report a shorter length
      // (only the amount not yet received).
      // The remaining data is already covered in the reported blocksize
      // when incrementing RCV.NXT
      length = res.length;
    }


    // make sure to mark the data as recveied (ACK) before putting in buffer,
    // since user callback can result in sending new data, which means we
    // want to ACK the data recv at the same time
    cb.RCV.NXT += length;
    // only actually recv the data if there is a read request (created with on_read)
    if(read_request != nullptr)
    {
      const auto recv = read_request->insert(in.seq(), in.tcp_data(), length, in.isset(PSH));
      // this ensures that the data we ACK is actually put in our buffer.
      Ensures(recv == length);
    }
  }
  // Packet out of order
  else if(( (in.seq() + in.tcp_data_length()) - cb.RCV.NXT) < cb.RCV.WND)
  {
    // only accept the data if we have a read request
    if(read_request != nullptr)
      recv_out_of_order(in);
  }


  // User callback didnt result in transmitting an ACK
  if(cb.SND.NXT == snd_nxt)
    ack_data();

  // [RFC 5681] ???
}

// This function need to sync both SACK and the read buffer, meaning:
// * Data cannot be old segments (already acked)
// * Data cannot be duplicate (already S-acked)
// * Read buffer needs to have room for the data
// * SACK list needs to have room for the entry (either connects or new)
//
// For now, only full segments are allowed (not partial),
// meaning the data will get thrown away if the read buffer not fully fits it.
// This makes everything much easier.
void Connection::recv_out_of_order(const Packet_view& in)
{
  // Packets before this point would totally ruin the buffer
  Expects((in.seq() - cb.RCV.NXT) < cb.RCV.WND);
  // We know that out of order packets wouldnt reach here
  // without SACK being permitted
  Expects(sack_perm);

  // The SACK list is initated on the first out of order packet
  if(UNLIKELY(not sack_list))
    sack_list = std::make_unique<Sack_list>();

  size_t length = in.tcp_data_length();
  auto seq = in.seq();

  // If it's already SACKed, it means we already received the data,
  // just ignore
  // note: Due to not accepting partial data i think we're safe with
  // not passing length as a second arg to contains
  if(UNLIKELY(sack_list->contains(seq))) {
    return;
  }

  auto fits = read_request->fits(seq);
  // TODO: if our packet partial fits, we just ignores it for now
  // to avoid headache
  if(fits >= length)
  {
    // Insert into SACK list before the buffer, since we already know 'fits'
    auto res = sack_list->recv_out_of_order(seq, length);

    // if we can't add the entry, we can't allow to insert into the buffer
    length = res.length;

    if(UNLIKELY(length == 0))
      return;

    const auto inserted = read_request->insert(seq, in.tcp_data(), length, in.isset(PSH));
    Ensures(inserted == length && "No partial insertion support");
    bytes_sacked_ += inserted;
  }

  /*
  size_t rem = length;
  // Support filling partial packets
  while(rem != 0 and fits != 0)
  {
    auto offset = length - rem;
    auto inserted = read_request->insert(seq, in.tcp_data()+offset, std::min(rem, fits), in.isset(PSH));

    // Assume for now that we have room in sack list
    auto res = sack_list->recv_out_of_order(seq, inserted);
    std::cout << res.entries;

    seq += inserted;
    rem -= inserted;

    fits = read_request->fits(seq);
  }*/
}

void Connection::ack_data()
{
  const auto snd_nxt = cb.SND.NXT;
  // ACK by trying to send more
  if (can_send())
  {
    writeq_push();
    // nothing got sent
    if (cb.SND.NXT == snd_nxt)
    {
      send_ack();
    }
    // something got sent
    else
    {
      dack_ = 0;
    }
  }
  // else regular ACK
  else
  {
    if (use_dack() and dack_ == 0)
    {
      start_dack();
    }
    else
    {
      stop_dack();
      send_ack();
    }
  }
}

void Connection::take_rtt_measure(const Packet_view& packet)
{
  if(cb.SND.TS_OK)
  {
    const auto* ts = packet.ts_option();
    // reparse to avoid case when stored ts suddenly get lost
    if(ts == nullptr)
      ts = packet.parse_ts_option();
    if(ts)
    {
      rttm.rtt_measurement(RTTM::milliseconds{host_.get_ts_value() - ntohl(ts->ecr)});
      return;
    }
  }

  if(rttm.active())
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
  else if(writeq.size())
  {
    auto& buf = writeq.una();

    // TODO: Finish to send window zero probe, but only on rtx timeout

    //printf("<Connection::retransmit> With data (wq.sz=%zu) buf.size=%zu buf.unacked=%zu SND.WND=%u CWND=%u\n",
    //       writeq.size(), buf->size(), buf->size() - writeq.acked(), cb.SND.WND, cb.cwnd);
    fill_packet(*packet, buf->data() + writeq.acked(), buf->size() - writeq.acked());
      packet->set_flag(PSH);
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
  //printf("<Connection::RTX@timeout> Timed out (RTO %lld ms). FS: %u usable=%u\n",
  //  rttm.rto_ms().count(), flight_size(), usable_window());

  signal_rtx_timeout();
  // experimental
  if(rto_limit_reached()) {
    debug("<TCP::Connection::rtx_timeout> RTX attempt limit reached, closing. rtx=%u syn_rtx=%u\n",
      rtx_attempt_, syn_rtx_);
    abort();
    return;
  }

  // retransmit SND.UNA
  retransmit();
  rtx_attempt_++;

  // "back off" timer
  rttm.RTO *= 2.0;

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
  if(rtx_attempt_ == 1)
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
  this->dack_ = 0;
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

void Connection::signal_connect(const bool success)
{
  // if read request was set before we got a seq number,
  // update the starting sequence number for the read buffer
  if(read_request and success)
    read_request->set_start(cb.RCV.NXT);

  if(on_connect_)
    (success) ? on_connect_(retrieve_shared()) : on_connect_(nullptr);

  // If no data event was registered we still want to start buffering here,
  // in case the user is not yet ready to subscribe to data.
  if (read_request == nullptr and success) {
    read_request.reset(
      new Read_request(this->cb.RCV.NXT, host_.min_bufsize(), host_.max_bufsize(), bufalloc.get()));
  }
}

void Connection::signal_close()
{
  if(UNLIKELY(close_signaled_))
    return;
  close_signaled_ = true;

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

  // make sure all our delegates are cleaned up (to avoid circular dependencies)
  on_connect_.reset();
  on_disconnect_.reset();
  on_close_.reset();
  recv_wnd_getter.reset();
  if(read_request) {
    read_request->on_read_callback.reset();
    read_request->on_data_callback.reset();
  }


  debug2("<Connection::clean_up> Call clean_up delg on %s\n", to_string().c_str());
  // clean up all other copies
  // either in TCP::listeners_ (open) or Listener::syn_queue_ (half-open)
  if(_on_cleanup_)
    _on_cleanup_(this);


  // if someone put a copy in this delg its their problem..
  //_on_cleanup_.reset();

  debug2("<Connection::clean_up> Succesfully cleaned up\n");
}

std::string Connection::TCB::to_string() const {
  char buffer[512];
  int len = snprintf(buffer, sizeof(buffer),
      "SND .UNA:%u .NXT:%u .WND:%u .UP:%u .WL1:%u .WL2:%u  ISS:%u\n"
      "RCV .NXT:%u .WND:%u .UP:%u  IRS=%u",
      SND.UNA, SND.NXT, SND.WND, SND.UP, SND.WL1, SND.WL2, ISS,
      RCV.NXT, RCV.WND, RCV.UP, IRS);
  return std::string(buffer, len);
}

void Connection::parse_options(const Packet_view& packet) {
  debug("<TCP::parse_options> Parsing options. Offset: %u, Options: %u \n",
        packet.offset(), packet.tcp_options_length());

  const uint8_t* opt = packet.tcp_options();

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
      break;
    }

    case Option::SACK_PERM:
    {
      if(UNLIKELY(option->length != sizeof(Option::opt_sack_perm)))
          throw TCPBadOptionException{Option::SACK_PERM, "length != 2"};

      if(UNLIKELY(!packet.isset(SYN)))
        throw TCPBadOptionException{Option::SACK_PERM, "Non-SYN packet"};

      if(host_.uses_SACK())
      {
        sack_perm = true;
      }

      opt += option->length;
      break;
    }

    default:
      opt += option->length;
      break;
    }
  }
}

void Connection::add_option(Option::Kind kind, Packet_view& packet) {

  switch(kind) {

  case Option::MSS: {
    packet.add_tcp_option<Option::opt_mss>(MSS());
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

  case Option::SACK_PERM: {
    packet.add_tcp_option<Option::opt_sack_perm>();
    break;
  }
  default:
    break;
  }
}

bool Connection::uses_window_scaling() const noexcept
{
  return host_.uses_wscale();
}

bool Connection::uses_timestamps() const noexcept
{
  return host_.uses_timestamps();
}

bool Connection::uses_SACK() const noexcept
{
  return host_.uses_SACK();
}

void Connection::drop(const Packet_view& packet, [[maybe_unused]]Drop_reason reason)
{
  /*printf("Drop %s %#.x RCV.WND: %u RCV.NXT %u alloc free: %zu flight size: %u SND.WND: %u \n",
    packet.to_string().c_str(), reason, cb.RCV.WND, cb.RCV.NXT, bufalloc->allocatable(), flight_size(), cb.SND.WND);*/
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
  //printf("<TCP::Connection::reduce_ssthresh> Slow start threshold reduced: %u\n",
  //  cb.ssthresh);
}

void Connection::fast_retransmit() {
  //printf("<TCP::Connection::fast_retransmit> Fast retransmit initiated.\n");
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
  //printf("<TCP::Connection::finish_fast_recovery> Finished Fast Recovery - Cwnd: %u\n", cb.cwnd);
}
