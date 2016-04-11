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

/*
  This is most likely used in a ACTIVE open
*/
Connection::Connection(TCP& host, Port local_port, Socket remote) :
  host_(host),
  local_port_(local_port),
  remote_(remote),
  state_(&Connection::Closed::instance()),
  prev_state_(state_),
  control_block(),
  read_request(),
  write_queue(),
  queued_(false),
  time_wait_started(0)
{

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
      write_queue.emplace(buffer, callback);
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

  while(has_doable_job() and packets) {
    auto& buf = write_queue.front().first;
    // segmentize the buffer into packets
    auto written = send(buf, packets);
    // advance the buffer
    buf.advance(written);
    debug2("<TCP::Connection::offer> Wrote %u bytes (%u remaining) with [%u] packets left and a usable window of %i.\n",
      written, buf.remaining, packets, usable_window());
    // if finished
    if(!buf.remaining) {
      // callback and remove object
      write_queue.front().second(buf.offset);
      write_queue.pop();
      debug("<TCP::Connection::offer> Request finished.\n");
    }
  }
  assert(packets >= 0);
  debug("<TCP::Connection::offer> Finished working offer with [%u] packets left and a queue of (%u) with a usable window of %i\n",
    packets, write_queue.size(), usable_window());
  return !has_doable_job();
}


size_t Connection::send(const char* buffer, size_t remaining, size_t& packet_count, bool PUSH) {
  assert(packet_count && remaining);
  size_t bytes_written{0};

  while(remaining and packet_count and usable_window() >= MSDS()) {
    // retreive a new packet
    auto packet = create_outgoing_packet();
    // reduce the amount of packets available by one
    packet_count--;
    // add the seq, ack and flag
    packet->set_seq(control_block.SND.NXT).set_ack(control_block.RCV.NXT).set_flag(ACK);
    // calculate how much the packet can be filled with
    auto packet_limit = std::min((uint32_t)MSDS() - packet->header_size(), (uint32_t)usable_window());
    // fill the packet with data from the request
    size_t written = packet->fill(buffer+bytes_written, std::min(packet_limit, remaining));
    // update local variables
    bytes_written += written;
    remaining -= written;

    debug2("<TCP::Connection::write_to_send_buffer> Packet Limit: %u - Written: %u"
          " - Remaining: %u - Packet count: %u, Window: %u\n",
           packet_limit, written, remaining, packet_count, usable_window());

    // If last packet, add PUSH.
    if(!remaining and PUSH)
      packet->set_flag(PSH);

    // Advance outgoing sequence number (SND.NXT) with the length of the data.
    control_block.SND.NXT += packet->data_length();
    // TODO: Replace with chaining
    transmit(packet);
  }
  return bytes_written;
}

void Connection::write_queue_push() {
  while(!write_queue.empty()) {
    auto& buf = write_queue.front().first;
    auto written = send(buf);
    buf.advance(written);
    if(buf.remaining)
      return;
    write_queue.front().second(buf.offset);
    write_queue.pop();
  }
}

void Connection::write_queue_reset() {
  while(!write_queue.empty()) {
    auto& job = write_queue.front();
    job.second(job.first.offset);
    write_queue.pop();
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

  packet->set_win(control_block.SND.WND);

  // Set SEQ and ACK - I think this is OK..
  packet->set_seq(control_block.SND.NXT).set_ack(control_block.RCV.NXT);
  debug("<TCP::Connection::create_outgoing_packet> Outgoing packet created: %s \n", packet->to_string().c_str());

  return packet;
}

void Connection::transmit(TCP::Packet_ptr packet) {
  debug("<TCP::Connection::transmit> Transmitting: %s \n", packet->to_string().c_str());
  host_.transmit(packet);
  // Don't think we would like to retransmit reset packets..?
  if(!packet->isset(RST))
    queue_retransmission(packet);
}

void Connection::retransmit(TCP::Packet_ptr packet) {
  debug("<TCP::Connection::retransmit> Retransmitting: %s \n", packet->to_string().c_str());
  host_.transmit(packet);
}

void Connection::queue_retransmission(TCP::Packet_ptr packet, size_t rt_attempt) {
  const size_t ATTEMPT_LIMIT = 3;
  auto self = shared_from_this();

  if(rt_attempt <= ATTEMPT_LIMIT) {
    hw::PIT::instance().onTimeout(RTO() * rt_attempt,
    [packet, self, rt_attempt]
    {
      // Packet hasnt been ACKed.
      if(packet->seq() > self->tcb().SND.UNA) {
        debug("<TCP::Connection::queue_retransmission@onTimeout> Packet unacknowledge, retransmitting...\n");
        if(rt_attempt == 1)
          self->segment_loss_detected();
        //packet->set_ack(self->tcb().RCV.NXT);
        self->retransmit(packet);
        self->queue_retransmission(packet, rt_attempt+1);
      } else {
        debug2("<TCP::Connection::queue_retransmission@onTimeout> Packet acknowledged %s \n", packet->to_string().c_str());
        // Signal user?
      }
    });
  }
  else {
    printf("<TCP::Connection::queue_retransmission> Give up already... Already tried %u times. Time to kill connection?\n",
      ATTEMPT_LIMIT);
  }
  debug2("<TCP::Connection::queue_retransmission> Packet queued for retransmission [%u] \n", retransmit_try);
}

size_t Connection::duplicate_ack(TCP::Seq ack) {
  // if its another duplicate, increment count
  if(dup_acks_.ACK == ack)
    return ++dup_acks_.count;

  // if not, set the new ack and set count to 1
  dup_acks_.ACK = ack;
  dup_acks_.count = 1;
  return dup_acks_.count;
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


/*
  Next compute a Smoothed Round Trip Time (SRTT) as:

  SRTT = ( ALPHA * SRTT ) + ((1-ALPHA) * RTT)

  and based on this, compute the retransmission timeout (RTO) as:

  RTO = min[UBOUND,max[LBOUND,(BETA*SRTT)]]

  where UBOUND is an upper bound on the timeout (e.g., 1 minute),
  LBOUND is a lower bound on the timeout (e.g., 1 second), ALPHA is
  a smoothing factor (e.g., .8 to .9), and BETA is a delay variance
  factor (e.g., 1.3 to 2.0).
*/
std::chrono::milliseconds Connection::RTO() const {
  return 1s;
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
      control_block.SND.MSS = mss;
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

