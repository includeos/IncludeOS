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
#define DEBUG
#define DEBUG2

#include <net/tcp.hpp>
#include <net/tcp_connection_states.hpp>

using namespace net;
using Connection = TCP::Connection;

using namespace std;

const TCP::Connection::RTTM::duration_t TCP::Connection::RTTM::CLOCK_G;

/*
  This is most likely used in a ACTIVE open
*/
Connection::Connection(TCP& host, Port local_port, Socket remote) :
  host_(host),
  local_port_(local_port),
  remote_(remote),
  state_(&Connection::Closed::instance()),
  prev_state_(state_),
  cb(),
  read_request(),
  writeq(),
  queued_(false),
  time_wait_started(0)
{
  setup_congestion_control();
}

/*
  This is most likely used in a PASSIVE open
*/
Connection::Connection(TCP& host, Port local_port)
  : Connection(host, local_port, TCP::Socket())
{

}

void Connection::read(ReadBuffer buffer, ReadCallback callback) {
  try {
    state_->receive(*this, buffer);
    read_request.callback = callback;
  }
  catch (TCPException err) {
    callback(buffer.buffer, buffer.size());
  }
}

size_t Connection::receive(const uint8_t* data, size_t n, bool PUSH) {
  // should not be called without an read request
  assert(read_request.buffer.capacity());
  assert(n);
  auto& buf = read_request.buffer;
  size_t received{0};
  while(n) {
    auto read = receive(buf, data+received, n);
    // nothing was read to buffer
    if(!buf.advance(read)) {
      // buffer should be full
      assert(buf.full());
      // signal the user
      read_request.callback(buf.buffer, buf.size());
      // renew the buffer, releasing the old one
      buf.renew();
    }
    n -= read;
    received += read;
  }
  // n shouldnt be negative
  assert(n == 0);

  // end of data, signal the user
  if(PUSH) {
    buf.push = PUSH;
    read_request.callback(buf.buffer, buf.size());
    // renew the buffer, releasing the old one
    buf.renew();
  }

  return received;
}


void Connection::write(WriteBuffer buffer, WriteCallback callback) {
  try {
    // try to write
    auto written = state_->send(*this, buffer);
    debug("<Connection::write> off=%u rem=%u  written=%u\n",
      buffer.offset, buffer.remaining, written);
    // put request in line
    writeq.push_back({buffer, callback});
    // if data was written, advance
    if(written) {
      writeq.advance(written);
    }
  }
  catch(TCPException err) {
    callback(0);
  }
}

void Connection::offer(size_t& packets) {
  Expects(packets);

  debug("<TCP::Connection::offer> %s got offered [%u] packets. Usable window is %u.\n",
        to_string().c_str(), packets, usable_window());

  // write until we either cant send more (window closes or no more in queue),
  // or we're out of packets.
  while(can_send() and packets)
  {
    auto packet = create_outgoing_packet();
    packets--;

    // get next request in writeq
    auto& buf = writeq.nxt();
    // fill the packet with data
    auto written = fill_packet(packet, (char*)buf.pos(), buf.remaining, cb.SND.NXT);
    cb.SND.NXT += packet->tcp_data_length();

    // advance the write q
    writeq.advance(written);

    debug2("<TCP::Connection::offer> Wrote %u bytes (%u remaining) with [%u] packets left and a usable window of %u.\n",
           written, buf.remaining, packets, usable_window());

    transmit(packet);
  }

  debug("<TCP::Connection::offer> Finished working offer with [%u] packets left and a queue of (%u) with a usable window of %i\n",
        packets, writeq.size(), usable_window());
}

size_t Connection::send(const char* buffer, size_t remaining, size_t& packets_avail) {

  size_t bytes_written = 0;
  debug("<Connection::send> Trying to send %u bytes. Starting with uw=%u packets=%u\n",
    remaining, usable_window(), packets_avail);

  std::vector<Packet_ptr> packets;

  while(remaining and usable_window() >= SMSS() and packets_avail)
  {
    auto packet = create_outgoing_packet();
    packets_avail--;

    auto written = fill_packet(packet, buffer+bytes_written, remaining, cb.SND.NXT);
    cb.SND.NXT += packet->tcp_data_length();

    bytes_written += written;
    remaining -= written;

    if(!remaining or usable_window() < SMSS() or !packets_avail)
      packet->set_flag(PSH);

    transmit(packet);
    //packets.push_back(packet);
  }
  /*Ensures(!packets.empty());
  // get first packet
  auto i = packets.begin();
  auto head = i++;
  // chain packets
  while(i != packets.end()) {
    (*head)->chain(*i);
    head = i++;
  }
  // set push
  (*head)->set_flag(PSH);

  // transmit first packet
  transmit(packets.front());*/

  debug("<Connection::send> Sent %u bytes. Finished with uw=%u packets=%u\n",
    bytes_written, usable_window(), packets_avail);

  return bytes_written;
}

/*
void Connection::set_window(Packet_ptr packet) {
  packet->set_win(cb.RCV.WND);
}

void Connection::make_flight_ready(Packet_ptr packet) {
  set_window(packet);

  // Set Source (local == the current connection)
  packet->set_source(local());
  // Set Destination (remote)
  packet->set_destination(remote_);

  // set correct sequence numbers
  packet->set_seq(cb.SND.NXT).set_ack(cb.RCV.NXT);
}*/

void Connection::writeq_push() {
  while(writeq.remaining_requests()) {
    auto& buf = writeq.nxt();
    auto written = host_.send(shared_from_this(), (char*)buf.pos(), buf.remaining);
    writeq.advance(written);
    if(buf.remaining)
      return;
  }
}

size_t Connection::fill_packet(Packet_ptr packet, const char* buffer, size_t n, Seq seq) {
  Expects(!packet->has_tcp_data());

  auto written = packet->fill(buffer, std::min(n, (size_t)SMSS()));

  packet->set_seq(seq).set_ack(cb.RCV.NXT).set_flag(ACK);

  Ensures(written <= n);

  return written;
}

void Connection::limited_tx() {

  auto packet = create_outgoing_packet();

  debug("<Connection::limited_tx> UW: %u CW: %u, FS: %u\n", usable_window(), cb.cwnd, flight_size());

  auto& buf = writeq.nxt();

  auto written = fill_packet(packet, (char*)buf.pos(), buf.remaining, cb.SND.NXT);
  cb.SND.NXT += packet->tcp_data_length();

  writeq.advance(written);

  transmit(packet);
}

void Connection::writeq_reset() {
  debug2("<Connection::writeq_reset> Reseting.\n");
  writeq.reset();
  if(rtx_timer.active)
    rtx_stop();
}

void Connection::open(bool active) {
  try {
    debug("<TCP::Connection::open> Trying to open Connection...\n");
    state_->open(*this, active);
  }
  // No remote host, or state isnt valid for opening.
  catch (TCPException e) {
    debug("<TCP::Connection::open> Cannot open Connection. \n");
    signal_error(e);
  }
}

void Connection::close() {
  debug("<TCP::Connection::close> Active close on connection. \n");
  try {
    state_->close(*this);
    if(is_state(Closed::instance()))
      signal_close();
  } catch(TCPException err) {
    signal_error(err);
  }
}

/*
  Local:Port Remote:Port (STATE)
*/
string Connection::to_string() const {
  ostringstream os;
  os << local().to_string() << " " << remote_.to_string() << " (" << state_->to_string() << ")";
  return os.str();
}

void Connection::segment_arrived(TCP::Packet_ptr incoming) {

  signal_packet_received(incoming);

  if(incoming->has_tcp_options()) {
    try {
      parse_options(incoming);
    }
    catch(TCPBadOptionException err) {
      debug("<TCP::Connection::receive> %s \n", err.what());
    }
  }

  // Let state handle what to do when incoming packet arrives, and modify the outgoing packet.
  switch(state_->handle(*this, incoming)) {
  case State::OK: {
    // Do nothing.
    break;
  }
  case State::CLOSED: {
    debug("<TCP::Connection::receive> (%s => %s) State handle finished with CLOSED. We're done, ask host() to delete the connection.\n",
      prev_state_->to_string().c_str(), state_->to_string().c_str());
    writeq_reset();
    signal_close();
    break;
  };
  case State::CLOSE: {
    debug("<TCP::Connection::receive> State handle finished with CLOSE. onDisconnect has been called, close the connection. \n");
    state_->close(*this);
    break;
  };
  }
}

bool Connection::is_listening() const {
  return is_state(Listen::instance());
}

Connection::~Connection() {
  // Do all necessary clean up.
  // Free up buffers etc.
  debug2("<TCP::Connection::~Connection> Bye bye... \n");
}


TCP::Packet_ptr Connection::create_outgoing_packet() {
  auto packet = std::static_pointer_cast<TCP::Packet>((host_.inet_).createPacket(0));
  //auto packet = host_.create_empty_packet();

  packet->init();
  // Set Source (local == the current connection)
  packet->set_source(local());
  // Set Destination (remote)
  packet->set_destination(remote_);

  packet->set_win(cb.RCV.WND);

  // Set SEQ and ACK - I think this is OK..
  packet->set_seq(cb.SND.NXT).set_ack(cb.RCV.NXT);
  debug("<TCP::Connection::create_outgoing_packet> Outgoing packet created: %s \n", packet->to_string().c_str());

  return packet;
}

void Connection::transmit(TCP::Packet_ptr packet) {
  if(!rttm.active and packet->end() == cb.SND.NXT) {
    //printf("<TCP::Connection::transmit> Starting RTT measurement.\n");
    rttm.start();
  }
  //if(packet->seq() + packet->tcp_data_length() != cb.SND.NXT)
  //printf("<TCP::Connection::transmit> rseq=%u rack=%u\n",
  //  packet->seq() - cb.ISS, packet->ack() - cb.IRS);
  debug2("<TCP::Connection::transmit> TX %s\n", packet->to_string().c_str());

  host_.transmit(packet);
  if(packet->should_rtx() and !rtx_timer.active) {
    rtx_start();
  }
}

bool Connection::can_send_one() {
  return send_window() >= SMSS() and writeq.remaining_requests();
}

bool Connection::can_send() {
  return (usable_window() >= SMSS()) and writeq.remaining_requests();
}

void Connection::send_much() {
  writeq_push();
}

bool Connection::handle_ack(TCP::Packet_ptr in) {
  // dup ack
  /*
    1. Same ACK as latest received
    2. outstanding data
    3. packet is empty
    4. is not an wnd update
  */
  if(in->ack() == cb.SND.UNA and flight_size()
    and !in->has_tcp_data() and cb.SND.WND == in->win()
    and !in->isset(SYN) and !in->isset(FIN))
  {
    dup_acks_++;
    on_dup_ack();
    return false;
  } // < dup ack

  // new ack
  else if(in->ack() >= cb.SND.UNA) {

    if( cb.SND.WL1 < in->seq() or ( cb.SND.WL1 == in->seq() and cb.SND.WL2 <= in->ack() ) )
    {
      cb.SND.WND = in->win();
      cb.SND.WL1 = in->seq();
      cb.SND.WL2 = in->ack();
      //printf("<Connection::handle_ack> Window update (%u)\n", cb.SND.WND);
    }

    acks_rcvd_++;

    debug("<Connection::handle_ack> New ACK#%u: %u FS: %u %s\n", acks_rcvd_,
      in->ack() - cb.ISS, flight_size(), fast_recovery ? "[RECOVERY]" : "");

    // [RFC 6582] p. 8
    prev_highest_ack_ = cb.SND.UNA;
    highest_ack_ = in->ack();

    // used for cwnd calculation (Reno)
    size_t bytes_acked = in->ack() - cb.SND.UNA;
    cb.SND.UNA = in->ack();

    // ack everything in rtx queue
    if(rtx_timer.active)
      rtx_ack(in->ack());

    // update cwnd when congestion avoidance?
    bool cong_avoid_rtt = false;

    // if measuring round trip time, stop
    if(rttm.active) {
      rttm.stop();
      cong_avoid_rtt = true;
    }

    // no fast recovery
    if(!fast_recovery) {
      //printf("<Connection::handle_ack> Not in Recovery\n");
      dup_acks_ = 0;
      cb.recover = cb.SND.NXT;

      // slow start
      if(cb.slow_start()) {
        reno_increase_cwnd(bytes_acked);
        debug2("<Connection::handle_ack> Slow start. cwnd=%u uw=%u\n",
          cb.cwnd, usable_window());
      }

      // congestion avoidance
      else {
        // increase cwnd once per RTT
        cb.cwnd += std::max(SMSS()*SMSS()/cb.cwnd, (uint32_t)1);
        debug2("<Connection::handle_ack> Congestion avoidance. cwnd=%u uw=%u\n",
          cb.cwnd, usable_window());
      } // < congestion avoidance

      // try to write
      //if(can_send() and acks_rcvd_ % 2 == 1)
      if(can_send())
        send_much();

      // if data, let state continue process
      if(in->has_tcp_data() or in->isset(FIN))
        return true;

    } // < !fast recovery

    // we're in fast recovery
    else {
      //printf("<Connection::handle_ack> In Recovery\n");
      // partial ack
      if(!reno_full_ack(in->ack())) {
        debug("<Connection::handle_ack> Partial ACK\n");
        reno_deflate_cwnd(bytes_acked);
        //printf("<TCP::Connection::handle_ack> Recovery - Partial ACK\n");
        retransmit();

        //dup_acks_ = 0;

        if(!reno_fpack_seen) {
          rtx_reset();
          reno_fpack_seen = true;
        }

        // send one segment if possible
        if(can_send()) {
          debug("<Connection::handle_ack> Sending one packet during recovery.\n");
          limited_tx();
        } else {
          debug("<Connection::handle_ack> Can't send during recovery - usable window is closed.\n");
        }

        if(in->has_tcp_data() or in->isset(FIN))
          return true;
      } // < partial ack

      // full ack
      else {
        debug("<Connection::handle_ack> Full ACK.\n");
        dup_acks_ = 0;
        finish_fast_recovery();
      } // < full ack

    } // < fast recovery

  } // < new ack

  // ACK outside
  else {
    return true;
  }
  return false;
}

/*
  Reno [RFC 5681] p. 9

  What to do when received ACK is a duplicate.
*/
void Connection::on_dup_ack() {
  debug2("<TCP::Connection::on_dup_ack> rack=%u i=%u\n", cb.SND.UNA - cb.ISS, dup_acks_);
  // if less than 3 dup acks
  if(dup_acks_ < 3) {

    if(limited_tx_) {
      // try to send one segment
      if(cb.SND.WND >= SMSS() and (flight_size() <= cb.cwnd + 2*SMSS()) and writeq.remaining_requests()) {
        limited_tx();
      }
    }
  }

  // 3 dup acks
  else if(dup_acks_ == 3) {
    debug("<TCP::Connection::on_dup_ack> Dup ACK == 3 - %u\n", cb.SND.UNA);

    if(cb.SND.UNA - 1 > cb.recover
      or ( congestion_window() > SMSS() and (highest_ack_ - prev_highest_ack_ <= 4*SMSS()) ))
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
    if(can_send())
      limited_tx();
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
void Connection::rtx_ack(const Seq ack) {
  auto acked = ack - prev_highest_ack_;
  // what if ack is from handshake / fin?
  writeq.acknowledge(acked);
  /*
    When all outstanding data has been acknowledged, turn off the
    retransmission timer.
  */
  if(cb.SND.UNA == cb.SND.NXT) {
    rtx_stop();
    rto_attempt = 0;
  }
  /*
    When an ACK is received that acknowledges new data, restart the
    retransmission timer so that it will expire after RTO seconds
    (for the current value of RTO).
  */
  else if(acked > 0) {
    rtx_reset();
    rto_attempt = 0;
  }

  //printf("<TCP::Connection::rt_acknowledge> ACK'ed %u packets. rtx_q: %u\n",
  //  x-rtx_q.size(), rtx_q.size());
}

/*
  Assumption
  Retransmission will only occur when one of the following are true:
  * There is data to be sent  (!SYN-SENT || !SYN-RCV) <- currently not supports
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
  if(is_state(SynSent::instance()) or is_state(SynReceived::instance())) {
    packet->set_flag(SYN);
    packet->set_seq(cb.SND.UNA);
    syn_rtx_++;
  }
  // If not, check if there is data and retransmit
  else if(writeq.size()) {
    auto& buf = writeq.una();
    fill_packet(packet, (char*)buf.pos(), buf.remaining, cb.SND.UNA);
  }
  // if no data
  else {
    packet->set_seq(cb.SND.UNA);
  }

  // If retransmission of a FIN packet
  if(is_state(FinWait1::instance()) or is_state(LastAck::instance())) {
    packet->set_flag(FIN);
  }

  //printf("<TCP::Connection::retransmit> rseq=%u \n", packet->seq() - cb.ISS);
  debug("<TCP::Connection::retransmit> RT %s\n", packet->to_string().c_str());
  host_.transmit(packet);
  /*
    Every time a packet containing data is sent (including a
    retransmission), if the timer is not running, start it running
    so that it will expire after RTO seconds (for the current value
    of RTO).
  */
  if(packet->should_rtx() and !rtx_timer.active) {
    rtx_start();
  }
}

void Connection::rtx_start() {
  Expects(!rtx_timer.active);
  auto i = rtx_timer.i;
  auto rto = rttm.RTO;
  rtx_timer.iter = hw::PIT::instance().on_timeout(rttm.RTO,
  [this, i, rto]
  {
    rtx_timer.active = false;
    debug("<TCP::Connection::RTX@timeout> %s Timed out (%f). FS: %u, i: %u rt_i: %u\n",
      to_string().c_str(), rto, flight_size(), i, rtx_timer.i);
    rtx_timeout();
  });
  rtx_timer.i++;
  rtx_timer.active = true;
}

void Connection::rtx_stop() {
  Expects(rtx_timer.active);
  hw::PIT::instance().stop_timer(rtx_timer.iter);
  rtx_timer.active = false;
}

void Connection::rtx_clear() {
  if(rtx_timer.active)
    rtx_stop();
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
  // experimental
  if(rto_limit_reached()) {
    printf("<TCP::Connection::rtx_timeout> RTX attempt limit reached, closing.\n");
    close();
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
    rttm.RTO = 3.0;
  }
  // timer need to be restarted
  if(!rtx_timer.active)
    rtx_start();

  /*
    [RFC 5681] p. 7

    When a TCP sender detects segment loss using the retransmission timer
    and the given segment has not yet been resent by way of the
    retransmission timer, the value of ssthresh MUST be set to no more
    than the value given in equation (4):

      ssthresh = max (FlightSize / 2, 2*SMSS)
  */
  if(rto_attempt++ == 0)
    reduce_ssthresh();

  /*
    [RFC 6582] p. 6
    After a retransmit timeout, record the highest sequence number
    transmitted in the variable recover, and exit the fast recovery
    procedure if applicable.
  */

  // update recover
  cb.recover = cb.SND.NXT;

  if(fast_recovery) // not sure if this is correct
    finish_fast_recovery();

  cb.cwnd = SMSS();

  /*
    NOTE: It's unclear which one comes first, or if finish_fast_recovery includes changing the cwnd.
  */
}

TCP::Seq Connection::generate_iss() {
  return host_.generate_iss();
}

void Connection::set_state(State& state) {
  prev_state_ = state_;
  state_ = &state;
  debug("<TCP::Connection::set_state> %s => %s \n",
        prev_state_->to_string().c_str(), state_->to_string().c_str());
}


void Connection::start_time_wait_timeout() {
  debug2("<TCP::Connection::start_time_wait_timeout> Time Wait timer started. \n");
  time_wait_started = OS::cycles_since_boot();
  auto timeout = 2 * host().MSL(); // 60 seconds
  // Passing "this"..?
  hw::PIT::instance().onTimeout(timeout,[this, timeout] {
      // The timer hasnt been updated
      if( OS::cycles_since_boot() >= (time_wait_started + timeout.count()) ) {
        signal_close();
      } else {
        debug2("<TCP::Connection::start_time_wait_timeout> time_wait_started has been updated. \n");
      }
    });
}

void Connection::signal_close() {
  debug("<TCP::Connection::signal_close> It's time to delete this connection. \n");
  host_.close_connection(*this);
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

void Connection::parse_options(TCP::Packet_ptr packet) {
  assert(packet->has_tcp_options());
  debug("<TCP::parse_options> Parsing options. Offset: %u, Options: %u \n",
        packet->offset(), packet->tcp_options_length());

  auto* opt = packet->tcp_options();

  while((char*)opt < packet->tcp_data()) {

    auto* option = (TCP::Option*)opt;

    switch(option->kind) {

    case Option::END: {
      return;
    }

    case Option::NOP: {
      opt++;
      break;
    }

    case Option::MSS: {
      // unlikely
      if(option->length != 4)
        throw TCPBadOptionException{Option::MSS, "length != 4"};
      // unlikely
      if(!packet->isset(SYN))
        throw TCPBadOptionException{Option::MSS, "Non-SYN packet"};

      auto* opt_mss = (Option::opt_mss*)option;
      uint16_t mss = ntohs(opt_mss->mss);
      cb.SND.MSS = mss;
      debug2("<TCP::parse_options@Option:MSS> MSS: %u \n", mss);
      opt += option->length;
      break;
    }

    default:
      return;
    }
  }
}

void Connection::add_option(TCP::Option::Kind kind, TCP::Packet_ptr packet) {

  switch(kind) {

  case Option::MSS: {
    packet->add_tcp_option<Option::opt_mss>(host_.MSS());
    debug2("<TCP::Connection::add_option@Option::MSS> Packet: %s - MSS: %u\n",
           packet->to_string().c_str(), ntohs(*(uint16_t*)(packet->tcp_options()+2)));
    break;
  }
  default:
    break;
  }
}
