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
      // reset the buffer
      buf.clear();
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
    // reset the buffer
    buf.clear();
  }

  return received;
}


void Connection::write(WriteBuffer buffer, WriteCallback callback) {
  try {
    auto written = state_->send(*this, buffer);
    buffer.advance(written);

    if(!buffer.remaining) {
      callback(buffer.offset);
    }
    else {
      writeq.emplace(buffer, callback);
    }
  }
  catch(TCPException err) {
    callback(0);
  }
}

bool Connection::offer(size_t& packets) {
  assert(packets);
  debug("<TCP::Connection::offer> %s got offered [%u] packets. Usable window is %i.\n",
        to_string().c_str(), packets, usable_window());

  while(has_doable_job() and packets and dup_acks_ < 3) {
    auto& buf = writeq.front().first;
    // segmentize the buffer into packets
    auto written = send(buf, packets, buf.remaining);
    // advance the buffer
    buf.advance(written);
    debug2("<TCP::Connection::offer> Wrote %u bytes (%u remaining) with [%u] packets left and a usable window of %i.\n",
           written, buf.remaining, packets, usable_window());
    // if finished
    if(!buf.remaining) {
      // callback and remove object
      writeq.front().second(buf.offset);
      writeq.pop();
      debug("<TCP::Connection::offer> Request finished.\n");
    }
  }
  assert(packets >= 0);
  debug("<TCP::Connection::offer> Finished working offer with [%u] packets left and a queue of (%u) with a usable window of %i\n",
        packets, writeq.size(), usable_window());
  return !has_doable_job() or dup_acks_ >= 3;
}

// TODO: This is starting to get complex and ineffective, refactor..
size_t Connection::send(const char* buffer, size_t remaining, size_t& packet_count, bool PUSH) {
  assert(packet_count);
  assert(remaining);
  size_t bytes_written{0};

  while(remaining and packet_count and usable_window() >= SMSS()) {
    // retreive a new packet
    auto packet = create_outgoing_packet();
    // reduce the amount of packets available by one
    packet_count--;
    // add the seq, ack and flag
    packet->set_seq(cb.SND.NXT).set_ack(cb.RCV.NXT).set_flag(ACK);
    // calculate how much the packet can be filled with
    auto packet_limit = std::min((uint32_t)MSDS() - packet->header_size(), (uint32_t)usable_window());
    // fill the packet with data from the request
    size_t written = packet->fill(buffer+bytes_written, std::min(packet_limit, remaining));
    // update local variables
    bytes_written += written;
    remaining -= written;

    // If last packet, add PUSH.
    // TODO: Redefine "push"
    //if((!remaining or !packet_count or usable_window() < SMSS()) and PUSH)
    if(!remaining and PUSH)
      packet->set_flag(PSH);

    // Advance outgoing sequence number (SND.NXT) with the length of the data.
    cb.SND.NXT += packet->data_length();
    // TODO: Replace with chaining
    transmit(packet);

    debug2("<TCP::Connection::send> Packet Limit: %u - Written: %u"
          " - Remaining: %u - Packet count: %u, Window: %u\n",
           packet_limit, written, remaining, packet_count, usable_window());
    if(reno_is_fast_recovering()) break;
  }
  debug("<TCP::Connection::send> Sent %u bytes of data\n", bytes_written);
  return bytes_written;
}

size_t Connection::send(const char* buffer, size_t n, Packet_ptr packet, bool PUSH) {
  auto written = packet->fill(buffer, n);
  packet->set_seq(cb.SND.NXT).set_ack(cb.RCV.NXT).set_flag(ACK);
  if(PUSH)
    packet->set_flag(PSH);
  cb.SND.NXT += packet->data_length();
  transmit(packet);
  return written;
}

void Connection::writeq_push() {
  while(!writeq.empty()) {
    auto& buf = writeq.front().first;
    auto written = send(buf);
    buf.advance(written);
    if(buf.remaining)
      return;
    writeq.front().second(buf.offset);
    writeq.pop();
  }
}

void Connection::limited_tx() {
  auto& buf = writeq.front().first;
  auto written = send((char*)buf.pos(), std::min(buf.remaining, (uint32_t)SMSS()), create_outgoing_packet(), false);
  //printf("<TCP::Connection::limited_tx> Limited transmit. %u written\n", written);
  buf.advance(written);
  if(buf.remaining)
    return;
  writeq.front().second(buf.offset);
  writeq.pop();
}

void Connection::writeq_reset() {
  while(!writeq.empty()) {
    auto& job = writeq.front();
    job.second(job.first.offset);
    writeq.pop();
  }
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

  if(incoming->has_options()) {
    try {
      parse_options(incoming);
    }
    catch(TCPBadOptionException err) {
      printf("<TCP::Connection::receive> %s \n", err.what());
    }
  }

  // Let state handle what to do when incoming packet arrives, and modify the outgoing packet.
  switch(state_->handle(*this, incoming)) {
  case State::OK: {
    // Do nothing.
    break;
  }
  case State::CLOSED: {
    debug("<TCP::Connection::receive> State handle finished with CLOSED. We're done, ask host() to delete the connection. \n");
    rtx_clear();
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
  auto packet = std::static_pointer_cast<TCP::Packet>((host_.inet_).createPacket(TCP::Packet::HEADERS_SIZE));
  //auto packet = host_.create_empty_packet();

  packet->init();
  // Set Source (local == the current connection)
  packet->set_source(local());
  // Set Destination (remote)
  packet->set_destination(remote_);

  packet->set_win(cb.SND.WND);

  // Set SEQ and ACK - I think this is OK..
  packet->set_seq(cb.SND.NXT).set_ack(cb.RCV.NXT);
  debug("<TCP::Connection::create_outgoing_packet> Outgoing packet created: %s \n", packet->to_string().c_str());

  return packet;
}

void Connection::transmit(TCP::Packet_ptr packet) {
  debug("<TCP::Connection::transmit> Transmitting: %s \n", packet->to_string().c_str());
  if(!rttm.active) {
    //printf("<TCP::Connection::transmit> Starting RTT measurement.\n");
    rttm.start();
  }

  host_.transmit(packet);
  if(packet->has_data())
    rtx_q.push_back(packet);
  if(!rtx_timer.active)
    rtx_start();
}
/*
void Connection::retransmit() {
  printf("<TCP::Connection::retransmit> \n");
}

void Connection::fast_retransmit() {
  printf("<TCP::Connection::fast_retransmit> \n");
  retransmit();

}

void Connection::finish_fast_recovery() {
  printf("<TCP::Connection::finish_fast_recovery> \n");
}

void Connection::on_rtx_timeout() {
  printf("<TCP::Connection::on_rtx_timeout> \n");
}

void Connection::limited_tx() {
  if(flight_size() <= )
}
*/


void Connection::handle_ack(TCP::Packet_ptr in) {
  // new ack
  if(in->ack() > cb.SND.UNA) {
    // [RFC 6582] p. 8
    //prev_highest_ack_ = cb.SND.UNA;
    //highest_ack_ = in->ack();

    if( cb.SND.WL1 < in->seq() or ( cb.SND.WL1 == in->seq() and cb.SND.WL2 <= in->ack() ) )
    {
      cb.SND.WND = in->win();
      cb.SND.WL1 = in->seq();
      cb.SND.WL2 = in->ack();
      debug2("<Connection::State::check_ack> Usable window slided (%i)\n", tcp.usable_window());
    }

    // used for cwnd calculation (Reno)
    size_t bytes_acked = in->ack() - cb.SND.UNA;
    cb.SND.UNA = in->ack();

    // ack everything in rtx queue
    rtx_ack(in->ack());

    // update cwnd when congestion avoidance?
    bool cong_avoid_rtt = false;

    // if measuring round trip time, stop
    if(rttm.active) {
      rttm.stop();
      cong_avoid_rtt = true;
    }

    // no fast recovery
    if(dup_acks_ < 3) {

      dup_acks_ = 0;

      // slow start
      if(cb.slow_start()) {
        reno_increase_cwnd(bytes_acked);
      }

      // congestion avoidance
      else {
        // increase cwnd once per RTT
        cb.cwnd += std::max(SMSS()*SMSS()/cb.cwnd, (uint32_t)1);
        //if(cong_avoid_rtt) {
          /*
            Not sure about this one..
            If timer is active, it means this ACK will stop the timer => one RTT.
            Not sure how this works with retransmission.
          */
          //reno_increase_cwnd(bytes_acked);
        //}
      } // < congestion avoidance

    } // < !fast recovery

    // we're in fast recovery
    else {

      // partial ack
      if(!reno_full_ack(in->ack())) {
        reno_deflate_cwnd(bytes_acked);
        //printf("<TCP::Connection::handle_ack> Recovery - Partial ACK\n");
        retransmit();

        if(!reno_fpack_seen) {
          rtx_reset();
          reno_fpack_seen = true;
        }

        // send one segment if possible
        if(can_send_one())
          limited_tx();
      } // < partial ack

      // full ack
      else {
        finish_fast_recovery();
      } // < full ack

    } // < fast recovery

  } // < new ack

  // dup ack
  /*
    1. Same ACK as latest received
    2. outstanding data
    3. packet is empty
    4. is not an wnd update
  */
  else if(in->ack() == cb.SND.UNA and flight_size()
    and !in->has_data() and cb.SND.WND == in->win())
  {
    dup_acks_++;
    on_dup_ack();
  } // < dup ack

  // ACK outside
  else {

  }
}


/*
  Reno [RFC 5681] p. 9

  What to do when received ACK is a duplicate.
*/
void Connection::on_dup_ack() {
  printf("<TCP::Connection::on_dup_ack> %u\n", dup_acks_);
  // if less than 3 dup acks
  if(dup_acks_ < 3) {

    // try to send one segment
    if(cb.SND.WND >= SMSS() and (flight_size() <= cb.cwnd + dup_acks_*SMSS()) and !writeq.empty())
      limited_tx();
  }

  // 3 dup acks
  else if(dup_acks_ == 3) {
    printf("<TCP::Connection::on_dup_ack> Dup ACK == 3 - %u\n", cb.SND.UNA);
    if(cb.SND.UNA - 1 > cb.recover) {
      cb.recover = cb.SND.NXT;
      printf("<TCP::Connection::on_dup_ack> Enter Recovery - Flight Size: %u\n", flight_size());
      fast_retransmit();
    }
  }

  // > 3 dup acks
  else {
    cb.cwnd += SMSS();
    // send one segment if possible
    if(can_send_one())
      limited_tx();
  }
}

/*
  As specified in [RFC3390], the SYN/ACK and the acknowledgment of the
  SYN/ACK MUST NOT increase the size of the congestion window.
  Further, if the SYN or SYN/ACK is lost, the initial window used by a
  sender after a correctly transmitted SYN MUST be one segment
  consisting of at most SMSS bytes.
*/
/*void Connection::acknowledge(Seq ACK) {
  DUP_ACK = 0;
  size_t bytes_acked = ACK - cb.SND.UNA;
  cb.SND.UNA = ACK;


  rtx_ack(ACK);

  reno_update_heuristic_ack(ACK);
  if(!reno_is_fast_recovering()) {
    if(reno_slow_start()) {
      reno_increase_cwnd(bytes_acked);
      debug2("<TCP::Connection::reno_ack> Slow start - cwnd increased: %u\n",
              cb.cwnd);
    }
    // congestion avoidance
    else {
      if(rttm.active) {
        reno_increase_cwnd(bytes_acked);
        debug2("<TCP::Connection::reno_ack> Congestion avoidance - cwnd increased: %u\n",
          cb.cwnd);
      }
    }
  }
  // fast recovery
  else {
    // full acknowledgement
    if(reno_full_ack(ACK)) {
      reno_exit_fast_recovery();
    }
    // partial acknowledgement
    else {
      //fast_rtx();
      retransmit();
      reno_deflate_cwnd(bytes_acked);
      if(RENO_FPACK_NOT_SEEN) {
        printf("<TCP::Connection::acknowledge> #1 Partial ACK - %u, Cwnd: %u\n", ACK, cb.cwnd);
        rt_restart();
        RENO_FPACK_NOT_SEEN = false;
      } else {
        //printf("<TCP::Connection::acknowledge> Partial ACK - %u, Cwnd: %u\n", ACK, cb.cwnd);
      }
      try_limited_tx();
    }
  }

  if(rttm.active)
    rttm.stop();
}*/

/*
  [RFC 6298]

   (5.2) When all outstanding data has been acknowledged, turn off the
         retransmission timer.

   (5.3) When an ACK is received that acknowledges new data, restart the
         retransmission timer so that it will expire after RTO seconds
         (for the current value of RTO).
*/
void Connection::rtx_ack(const Seq ack) {
  auto x = rtx_q.size();
  while(!rtx_q.empty()) {
    if(rtx_q.front()->is_acked_by(ack))
      rtx_q.pop_front();
    else
      break;
  }
  /*
    When all outstanding data has been acknowledged, turn off the
    retransmission timer.
  */
  if(rtx_q.empty() and rtx_timer.active) {
    rtx_stop();
  }
  /*
    When an ACK is received that acknowledges new data, restart the
    retransmission timer so that it will expire after RTO seconds
    (for the current value of RTO).
  */
  else if(x - rtx_q.size() > 0) {
    rto_attempt = 0;
    rtx_reset();
  }
  //printf("<TCP::Connection::rt_acknowledge> ACK'ed %u packets. rtx_q: %u\n",
  //  x-rtx_q.size(), rtx_q.size());
}


void Connection::retransmit() {
  if(rtx_q.empty())
    return;
  auto packet = rtx_q.front();
  printf("<TCP::Connection::retransmit> Retransmitting: %u \n", packet->seq());
  host_.transmit(packet);
  /*
    Every time a packet containing data is sent (including a
    retransmission), if the timer is not running, start it running
    so that it will expire after RTO seconds (for the current value
    of RTO).
  */
  //if(!rt_timer.active)
  //  rt_start();
}

void Connection::rtx_start() {
  assert(!rtx_timer.active);
  auto i = rtx_timer.i;
  rtx_timer.iter = hw::PIT::instance().on_timeout(rttm.RTO,
  [this, i]
  {
    rtx_timer.active = false;
    printf("<TCP::Connection::RTO@timeout> %s Timed out. rt_q: %u, i: %u rt_i: %u\n",
      to_string().c_str(), rtx_q.size(), i, rtx_timer.i);
    rtx_timeout();
  });
  rtx_timer.i++;
  rtx_timer.active = true;
}

void Connection::rtx_stop() {
  assert(rtx_timer.active);
  hw::PIT::instance().stop_timer(rtx_timer.iter);
  rtx_timer.active = false;
}

void Connection::rtx_flush() {
  while(!rtx_q.empty()) {
    host_.transmit(rtx_q.front());
    rtx_q.pop_front();
  }
}

void Connection::rtx_clear() {
  if(rtx_timer.active)
    rtx_stop();
  rtx_q.clear();
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
  // retransmit SND.UNA
  retransmit();

  if(!rtx_q.front()->isset(SYN)) {
    // "back off" timer
    rttm.RTO *= 2;
  }
  // we never queue SYN packets since they don't carry data..
  else {
    rttm.RTO = 3.0;
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

  if(dup_acks_ >= 3) // not sure if this is correct
    finish_fast_recovery();

  cb.cwnd = SMSS();

  /*
    NOTE: It's unclear which one comes first, or if finish_fast_recovery includes chaining the cwnd.
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
  assert(packet->has_options());
  debug("<TCP::parse_options> Parsing options. Offset: %u, Options: %u \n",
        packet->offset(), packet->options_length());

  auto* opt = packet->options();

  while((char*)opt < packet->data()) {

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
    packet->add_option<Option::opt_mss>(host_.MSS());
    debug2("<TCP::Connection::add_option@Option::MSS> Packet: %s - MSS: %u\n",
           packet->to_string().c_str(), ntohs(*(uint16_t*)(packet->options()+2)));
    break;
  }
  default:
    break;
  }
}
