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

#include <gsl/gsl_assert> // Ensures/Expects
#include <net/tcp/connection_states.hpp>
#include <net/tcp/packet.hpp>
#include <sstream>

using namespace net::tcp;
using namespace std;

/////////////////////////////////////////////////////////////////////
/*
  COMMON STATE FUNCTIONS

  These functions are helper functions and used by more than one state.
*/
/////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////
/*
  1. Check Sequence number.

  Used for checking if the sequence number is acceptable.

  [RFC 793]:

  SYN-RECEIVED STATE
  ESTABLISHED STATE
  FIN-WAIT-1 STATE
  FIN-WAIT-2 STATE
  CLOSE-WAIT STATE
  CLOSING STATE
  LAST-ACK STATE
  TIME-WAIT STATE

  Segments are processed in sequence.  Initial tests on arrival
  are used to discard old duplicates, but further processing is
  done in SEG.SEQ order.  If a segment's contents straddle the
  boundary between old and new, only the new parts should be
  processed.

  There are four cases for the acceptability test for an incoming
  segment:

  Segment Receive  Test
  Length  Window
  ------- -------  -------------------------------------------

  0       0     SEG.SEQ = RCV.NXT

  0      >0     RCV.NXT =< SEG.SEQ < RCV.NXT+RCV.WND

  >0       0     not acceptable

  >0      >0     RCV.NXT =< SEG.SEQ < RCV.NXT+RCV.WND
  or RCV.NXT =< SEG.SEQ+SEG.LEN-1 < RCV.NXT+RCV.WND

  If the RCV.WND is zero, no segments will be acceptable, but
  special allowance should be made to accept valid ACKs, URGs and
  RSTs.

  If an incoming segment is not acceptable, an acknowledgment
  should be sent in reply (unless the RST bit is set, if so drop
  the segment and return):

  <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK>

  After sending the acknowledgment, drop the unacceptable segment
  and return.

  In the following it is assumed that the segment is the idealized
  segment that begins at RCV.NXT and does not exceed the window.
  One could tailor actual segments to fit this assumption by
  trimming off any portions that lie outside the window (including
  SYN and FIN), and only processing further if the segment then
  begins at RCV.NXT.  Segments with higher begining sequence
  numbers may be held for later processing.
*/
/////////////////////////////////////////////////////////////////////

// TODO: Optimize this one. It checks for the same things.
bool Connection::State::check_seq(Connection& tcp, Packet_ptr in) {
  auto& tcb = tcp.tcb();
  bool acceptable = false;
  debug2("<Connection::State::check_seq> TCB: %s \n",tcb.to_string().c_str());
  // #1
  if( in->seq() == tcb.RCV.NXT ) {
    acceptable = true;
  }
  // #2
  else if( tcb.RCV.NXT <= in->seq() and in->seq() < tcb.RCV.NXT + tcb.RCV.WND ) {
    acceptable = true;
  }
  // #3 (INVALID)
  else if( in->seq() + in->tcp_data_length()-1 > tcb.RCV.NXT+tcb.RCV.WND ) {
    acceptable = false;
  }
  // #4
  else if( (tcb.RCV.NXT <= in->seq() and in->seq() < tcb.RCV.NXT + tcb.RCV.WND)
           or ( tcb.RCV.NXT <= in->seq()+in->tcp_data_length()-1 and in->seq()+in->tcp_data_length()-1 < tcb.RCV.NXT+tcb.RCV.WND ) ) {
    acceptable = true;
  }
  /*
    If an incoming segment is not acceptable, an acknowledgment
    should be sent in reply (unless the RST bit is set, if so drop
    the segment and return):

    <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK>

    After sending the acknowledgment, drop the unacceptable segment
    and return.
  */
  if(!acceptable) {
    if(!in->isset(RST)) {
      auto packet = tcp.outgoing_packet();
      packet->set_seq(tcb.SND.NXT).set_ack(tcb.RCV.NXT).set_flag(ACK);
      tcp.transmit(packet);
    }
    std::stringstream ss;
    ss << "Unacceptable SEQ: "
       << "[Packet: SEQ: " << in->seq() << " LEN: " << in->tcp_data_length() << "] "
       << "[TCB: RCV.NXT: " << tcb.RCV.NXT << " RCV.WND: " << tcb.RCV.WND << "]";

    tcp.drop(in, ss.str());
    return false;
  }
  debug2("<Connection::State::check_seq> Acceptable SEQ: %u \n", in->seq());
  // is acceptable.
  return true;
}
/////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////
/*
  4. Check SYN

  Used to filter out packets carrying SYN-flag when they're not supposed to.

  [RFC 793]:

  SYN-RECEIVED
  ESTABLISHED STATE
  FIN-WAIT STATE-1
  FIN-WAIT STATE-2
  CLOSE-WAIT STATE
  CLOSING STATE
  LAST-ACK STATE
  TIME-WAIT STATE

  If the SYN is in the window it is an error, send a reset, any
  outstanding RECEIVEs and SEND should receive "reset" responses,
  all segment queues should be flushed, the user should also
  receive an unsolicited general "connection reset" signal, enter
  the CLOSED state, delete the TCB, and return.

  If the SYN is not in the window this step would not be reached
  and an ack would have been sent in the first step (sequence
  number check).
*/
/////////////////////////////////////////////////////////////////////

void Connection::State::unallowed_syn_reset_connection(Connection& tcp, Packet_ptr in) {
  assert(in->isset(SYN));
  debug("<Connection::State::unallowed_syn_reset_connection> Unallowed SYN for STATE: %s, reseting connection.\n",
        tcp.state().to_string().c_str());
  // Not sure if this is the correct way to send a "reset response"
  auto packet = tcp.outgoing_packet();
  packet->set_seq(in->ack()).set_flag(RST);
  tcp.transmit(packet);
  tcp.signal_disconnect(Disconnect::RESET);
}

/////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////
/*
  5. Check ACK

  "Process" the packet if ACK is present. If not, drop the packet.

  [RFC 793] Page 72-73.
*/
/////////////////////////////////////////////////////////////////////

bool Connection::State::check_ack(Connection& tcp, Packet_ptr in) {
  debug2("<Connection::State::check_ack> Checking for ACK in STATE: %s \n", tcp.state().to_string().c_str());
  if( in->isset(ACK) ) {
    auto& tcb = tcp.tcb();
    /*
      If SND.UNA < SEG.ACK =< SND.NXT then, set SND.UNA <- SEG.ACK.
      Any segments on the retransmission queue which are thereby
      entirely acknowledged are removed.  Users should receive
      positive acknowledgments for buffers which have been SENT and
      fully acknowledged (i.e., SEND buffer should be returned with
      "ok" response).  If the ACK is a duplicate
      (SEG.ACK < SND.UNA), it can be ignored.  If the ACK acks
      something not yet sent (SEG.ACK > SND.NXT) then send an ACK,
      drop the segment, and return.
    */
    // Correction: [RFC 1122 p. 94]
    // ACK is inside sequence space
    if(in->ack() <= tcb.SND.NXT ) {

      return tcp.handle_ack(in);
      // this is a "new" ACK
      //if(tcb.SND.UNA <= in->ack()) {

        // this is a NEW ACK
        //if(tcb.SND.UNA < in->ack())
        //{
        //  tcp.acknowledge(in->ack());
        //}
         // [RFC 5681]
        /*
          DUPLICATE ACKNOWLEDGMENT:
          An acknowledgment is considered a
          "duplicate" in the following algorithms when
          (a) the receiver of the ACK has outstanding data
          (b) the incoming acknowledgment carries no data
          (c) the SYN and FIN bits are both off
          (d) the acknowledgment number is equal to the greatest acknowledgment
          received on the given connection (TCP.UNA from [RFC793]) and
          (e) the advertised window in the incoming acknowledgment equals the
          advertised window in the last incoming acknowledgment.

          Note that a sender using SACK [RFC2018] MUST NOT send
          new data unless the incoming duplicate acknowledgment contains
          new SACK information.
        */
        // this is a RFC 5681 DUP ACK
        //!in->isset(FIN) and !in->isset(SYN)
        //else if(tcp.reno_is_dup_ack(in)) {
        //  debug2("<Connection::State::check_ack> Reno Dup ACK %u\n", in->ack());
        //  tcp.reno_dup_ack(in->ack());
        //}
        // this is an RFC 793 DUP ACK
        //else {
          //printf("<Connection::State::check_ack> RFC 793 Dup ACK %u\n", in->ack());
        //}
      //}
      // this is an "old" ACK out of order
      //else {
      //  printf("<Connection::State::check_ack> ACK out of order (SND.UNA > ACK)\n");
      //}
      // tcp.signal_sent();
      // return that buffer has been SENT - currently no support to receipt sent buffer.
    }
    /* If the ACK acks something not yet sent (SEG.ACK > SND.NXT) then send an ACK, drop the segment, and return. */
    else {
      auto packet = tcp.outgoing_packet();
      packet->set_flag(ACK);
      tcp.transmit(packet);
      tcp.drop(in, "ACK > SND.NXT");
      return false;
    }
    return true;
  }
  // ACK not set.
  else {
    tcp.drop(in, "!ACK");
    return false;
  }
}

/////////////////////////////////////////////////////////////////////
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
/////////////////////////////////////////////////////////////////////

void Connection::State::process_segment(Connection& tcp, Packet_ptr in) {
  assert(in->has_tcp_data());

  auto& tcb = tcp.tcb();
  auto length = in->tcp_data_length();
  // Receive could result in a user callback. This is used to avoid sending empty ACK reply.
  debug("<Connection::State::process_segment> Received packet with DATA-LENGTH: %i. Add to receive buffer. \n", length);
  tcb.RCV.NXT += length;
  auto snd_nxt = tcb.SND.NXT;
  if(tcp.read_request.buffer.capacity()) {
    auto received = tcp.receive((uint8_t*)in->tcp_data(), in->tcp_data_length(), in->isset(PSH));
    Ensures(received == length);
  }

  // [RFC 5681]
  //tcb.SND.cwnd += std::min(length, tcp.SMSS());
  debug2("<Connection::State::process_segment> Advanced RCV.NXT: %u. SND.NXT = %u \n", tcb.RCV.NXT, snd_nxt);

  if(tcb.SND.NXT == snd_nxt) {
    auto packet = tcp.outgoing_packet();
    packet->set_seq(tcb.SND.NXT).set_ack(tcb.RCV.NXT).set_flag(ACK);
    tcp.transmit(packet);
  }
  //if(tcp.can_send())
  //  tcp.send_much();
  /*if(tcp.has_doable_job() and !tcp.is_queued()) {
    printf("<Connection::State::process_segment> Usable window: %i\n", tcp.usable_window());
    tcp.writeq_push();
  }*/

  /*
    WARNING/NOTE:
    Not sure how "dangerous" the following is, and how big of a bottleneck it is.
    Maybe has to be implemented with timers or something.
  */

  /*
    Once the TCP takes responsibility for the data it advances
    RCV.NXT over the data accepted, and adjusts RCV.WND as
    apporopriate to the current buffer availability.  The total of
    RCV.NXT and RCV.WND should not be reduced.
  */
  // no data has been sent during user callback
  // TODO: A lot of cleanup / refactoring - this is messy.
  /*if(snd_nxt == tcb.SND.NXT) {
    // Piggyback ACK with outgoing data
    if(tcp.has_doable_job() and !tcp.is_queued()) {
      debug2("<Connection::State::process_segment> Usable window: %i\n", tcp.usable_window());
      tcp.writeq_push();
      // we tried to push data, but nothing was written, reply the sender immediately
      if(tcp.usable_window() == tcb.SND.WND) {
        auto packet = tcp.outgoing_packet();
        packet->set_seq(tcb.SND.NXT).set_ack(tcb.RCV.NXT).set_flag(ACK);
        tcp.transmit(packet);
      }
    }
    // TODO: Selective ACK
    // If no outgoing data right now - reply with ACK.
    else {
      debug2("<Connection::State::process_segment> ACK. Window: %i, Queue: %u, is_queued: %s\n",
             tcp.usable_window(), tcp.writeq.size(), tcp.is_queued() ? "true" : "false");
      auto packet = tcp.outgoing_packet();
      packet->set_seq(tcb.SND.NXT).set_ack(tcb.RCV.NXT).set_flag(ACK);
      tcp.transmit(packet);
    }
  }
  else {
    debug2("<Connection::State::process_segment> SND.NXT > snd_nxt, this packet has already been acknowledged. \n");
  }*/
}
/////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////
/*
  8. Process FIN

  Process a packet with FIN, by signal disconnect, reply with ACK etc.

  [RFC 793] Page 75:

  If the FIN bit is set, signal the user "connection closing" and
  return any pending RECEIVEs with same message, advance RCV.NXT
  over the FIN, and send an acknowledgment for the FIN.  Note that
  FIN implies PUSH for any segment text not yet delivered to the
  user.
*/
/////////////////////////////////////////////////////////////////////

void Connection::State::process_fin(Connection& tcp, Packet_ptr in) {
  debug("<Connection::State::process_fin> Processing FIN bit in STATE: %s \n", tcp.state().to_string().c_str());
  assert(in->isset(FIN));
  auto& tcb = tcp.tcb();
  // Advance RCV.NXT over the FIN?
  tcb.RCV.NXT++;
  //auto fin = in->tcp_data_length();
  //tcb.RCV.NXT += fin;
  auto snd_nxt = tcb.SND.NXT;
  // empty the read buffer
  if(!tcp.read_request.buffer.empty())
    tcp.receive_disconnect();
  // signal disconnect to the user
  tcp.signal_disconnect(Disconnect::CLOSING);

  // only ack FIN if user callback didn't result in a sent packet
  if(tcb.SND.NXT == snd_nxt) {
    debug2("<Connection::State::process_fin> acking FIN\n");
    auto packet = tcp.outgoing_packet();
    packet->set_ack(tcb.RCV.NXT).set_flag(ACK);
    tcp.transmit(packet);
  }

}
/////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////
/*
  Send a reset segment. Used when aborting a connection.

  [RFC 793]:
  Send a reset segment:

  <SEQ=SND.NXT><CTL=RST>

  All queued SENDs and RECEIVEs should be given "connection reset"
  notification; all segments queued for transmission (except for the
  RST formed above) or retransmission should be flushed, delete the
  TCB, enter CLOSED state, and return.
*/
/////////////////////////////////////////////////////////////////////

void Connection::State::send_reset(Connection& tcp) {
  tcp.writeq_reset();
  auto packet = tcp.outgoing_packet();
  packet->set_seq(tcp.tcb().SND.NXT).set_ack(0).set_flag(RST);
  tcp.transmit(packet);
}
/////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////
/*
  STATE IMPLEMENTATION

  Here follows the implementation for all the different State-Event combinations.

  The implemenation is ordered in the following structure:
  1. Function
  2. State

  Function order:
  OPEN
  SEND
  RECEIVE
  CLOSE
  ABORT
  HANDLE (SEGMENT ARRIVES)

  State order:
  State - this is the base state, works as fallback.
  Closed
  Listen
  SynSent
  SynReceived
  Established
  FinWait1
  FinWait2
  CloseWait
  Closing
  LastAck
  TimeWait
*/
/////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////
/*
  OPEN
*/
/////////////////////////////////////////////////////////////////////

void Connection::State::open(Connection&, bool) {
  throw TCPException{"Connection already exists."};
}

void Connection::Closed::open(Connection& tcp, bool active) {
  if(active) {
    // There is a remote host
    if(!tcp.remote().is_empty()) {
      auto& tcb = tcp.tcb();
      tcb.init();
      auto packet = tcp.outgoing_packet();
      packet->set_seq(tcb.ISS).set_flag(SYN);

      /*
        Add MSS option.
      */
      tcp.add_option(Option::MSS, packet);

      tcb.SND.UNA = tcb.ISS;
      tcb.SND.NXT = tcb.ISS+1;
      tcp.transmit(packet);
      tcp.set_state(SynSent::instance());
    } else {
      throw TCPException{"No remote host set."};
    }
  } else {
    tcp.set_state(Connection::Listen::instance());
  }
}

void Connection::Listen::open(Connection& tcp, bool) {
  if(!tcp.remote().is_empty()) {
    auto& tcb = tcp.tcb();
    tcb.init();
    auto packet = tcp.outgoing_packet();
    packet->set_seq(tcb.ISS).set_flag(SYN);
    tcb.SND.UNA = tcb.ISS;
    tcb.SND.NXT = tcb.ISS+1;
    tcp.transmit(packet);
    tcp.set_state(SynSent::instance());
  } else {
    throw TCPException{"No remote host set."};
  }
}


/////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////
/*
  SEND
*/
/////////////////////////////////////////////////////////////////////

size_t Connection::State::send(Connection&, WriteBuffer&) {
  throw TCPException{"Connection closing."};
}

size_t Connection::Closed::send(Connection&, WriteBuffer&) {
  throw TCPException{"Connection does not exist."};
}

size_t Connection::Listen::send(Connection&, WriteBuffer&) {
  // TODO: Skip this?
  /*
    If the foreign socket is specified, then change the connection
    from passive to active, select an ISS.  Send a SYN segment, set
    SND.UNA to ISS, SND.NXT to ISS+1.  Enter SYN-SENT state.  Data
    associated with SEND may be sent with SYN segment or queued for
    transmission after entering ESTABLISHED state.  The urgent bit if
    requested in the command must be sent with the data segments sent
    as a result of this command.  If there is no room to queue the
    request, respond with "error:  insufficient resources".  If
    Foreign socket was not specified, then return "error:  foreign
    socket unspecified".
  */
  //if(tcp.remote().is_empty())
  //  throw TCPException{"Foreign socket unspecified."};
  throw TCPException{"Cannot send on listening connection."};
  return 0;
}

size_t Connection::SynSent::send(Connection&, WriteBuffer&) {
  /*
    Queue the data for transmission after entering ESTABLISHED state.
    If no space to queue, respond with "error:  insufficient
    resources".
  */

  return 0; // nothing written, indicates queue
}

size_t Connection::SynReceived::send(Connection&, WriteBuffer&) {
  /*
    Queue the data for transmission after entering ESTABLISHED state.
    If no space to queue, respond with "error:  insufficient
    resources".
  */

  return 0; // nothing written, indicates queue
}

size_t Connection::Established::send(Connection& tcp, WriteBuffer& buffer) {
  // if nothing in queue, try to write directly
  if(!tcp.writeq.remaining_requests())
    return tcp.send(buffer);

  return 0;
}

size_t Connection::CloseWait::send(Connection& tcp, WriteBuffer& buffer) {
  // if nothing in queue, try to write directly
  if(!tcp.writeq.remaining_requests())
    return tcp.send(buffer);

  return 0;
}

/////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////
/*
  RECEIVE
*/
/////////////////////////////////////////////////////////////////////

void Connection::State::receive(Connection&, ReadBuffer&) {
  throw TCPException{"Connection closing."};
}

void Connection::Established::receive(Connection& tcp, ReadBuffer& buffer) {
  tcp.receive(buffer);
}

void Connection::FinWait1::receive(Connection& tcp, ReadBuffer& buffer) {
  tcp.receive(buffer);
}

void Connection::FinWait2::receive(Connection& tcp, ReadBuffer& buffer) {
  tcp.receive(buffer);
}

void Connection::CloseWait::receive(Connection& tcp, ReadBuffer& buffer) {
  tcp.receive(buffer);
}

/////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////
/*
  CLOSE
*/
/////////////////////////////////////////////////////////////////////

void Connection::State::close(Connection&) {
  throw TCPException{"Connection closing."};
}

void Connection::Listen::close(Connection& tcp) {
  /*
    Any outstanding RECEIVEs are returned with "error:  closing"
    responses.  Delete TCB, enter CLOSED state, and return.
  */
  // tcp.signal_disconnect("Closing")
  tcp.set_state(Closed::instance());
}

void Connection::SynSent::close(Connection& tcp) {
  /*
    Delete the TCB and return "error:  closing" responses to any
    queued SENDs, or RECEIVEs.
  */
  // tcp.signal_disconnect("Closing")
  tcp.set_state(Closed::instance());
}

void Connection::SynReceived::close(Connection& tcp) {
  /*
    If no SENDs have been issued and there is no pending data to send,
    then form a FIN segment and send it, and enter FIN-WAIT-1 state;
    otherwise queue for processing after entering ESTABLISHED state.
  */
  // Dont know how to queue for close for processing...
  auto& tcb = tcp.tcb();
  auto packet = tcp.outgoing_packet();
  packet->set_seq(tcb.SND.NXT++).set_ack(tcb.RCV.NXT).set_flags(ACK | FIN);
  tcp.transmit(packet);
  tcp.set_state(Connection::FinWait1::instance());
}

void Connection::Established::close(Connection& tcp) {
  auto& tcb = tcp.tcb();
  auto packet = tcp.outgoing_packet();
  packet->set_seq(tcb.SND.NXT++).set_ack(tcb.RCV.NXT).set_flags(ACK | FIN);
  tcp.transmit(packet);
  tcp.set_state(Connection::FinWait1::instance());
}

void Connection::FinWait1::close(Connection&) {
  /*
    Strictly speaking, this is an error and should receive a "error:
    connection closing" response.  An "ok" response would be
    acceptable, too, as long as a second FIN is not emitted (the first
    FIN may be retransmitted though).
  */
}

void Connection::FinWait2::close(Connection&) {
  /*
    Strictly speaking, this is an error and should receive a "error:
    connection closing" response.  An "ok" response would be
    acceptable, too, as long as a second FIN is not emitted (the first
    FIN may be retransmitted though).
  */
}

void Connection::CloseWait::close(Connection& tcp) {
  /*
    Queue this request until all preceding SENDs have been
    segmentized; then send a FIN segment, enter CLOSING state.
  */
  auto& tcb = tcp.tcb();
  auto packet = tcp.outgoing_packet();
  packet->set_seq(tcb.SND.NXT++).set_ack(tcb.RCV.NXT).set_flags(ACK | FIN);
  tcp.transmit(packet);
  // Correction: [RFC 1122 p. 93]
  tcp.set_state(Connection::LastAck::instance());
}

/////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////
/*
  ABORT
*/
/////////////////////////////////////////////////////////////////////

void Connection::State::abort(Connection&) {
  // Do nothing.
}

void Connection::SynReceived::abort(Connection& tcp) {
  send_reset(tcp);
}

void Connection::Established::abort(Connection& tcp) {
  send_reset(tcp);
}

void Connection::FinWait1::abort(Connection& tcp) {
  send_reset(tcp);
}

void Connection::FinWait2::abort(Connection& tcp) {
  send_reset(tcp);
}

void Connection::CloseWait::abort(Connection& tcp) {
  send_reset(tcp);
}

/////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////
/*
  HANDLE (SEGMENT ARRIVES)
*/
/////////////////////////////////////////////////////////////////////

State::Result Connection::Closed::handle(Connection& tcp, Packet_ptr in) {
  if(in->isset(RST)) {
    return OK;
  }
  auto packet = tcp.outgoing_packet();
  if(!in->isset(ACK)) {
    packet->set_seq(0).set_ack(in->seq() + in->tcp_data_length()).set_flags(RST | ACK);
  } else {
    packet->set_seq(in->ack()).set_flag(RST);
  }
  tcp.transmit(packet);
  return OK;
}


State::Result Connection::Listen::handle(Connection& tcp, Packet_ptr in) {
  if(in->isset(RST)) {
    // ignore
    return OK;
  }
  if(in->isset(ACK)) {
    auto packet = tcp.outgoing_packet();
    packet->set_seq(in->ack()).set_flag(RST);
    tcp.transmit(packet);
    return OK;
  }
  if(in->isset(SYN)) {
    auto& tcb = tcp.tcb();
    tcb.RCV.NXT   = in->seq()+1;
    tcb.IRS     = in->seq();
    tcb.init();
    tcb.SND.NXT   = tcb.ISS+1;
    tcb.SND.UNA   = tcb.ISS;
    debug("<Connection::Listen::handle> Received SYN Packet: %s TCB Updated:\n %s \n",
          in->to_string().c_str(), tcp.tcb().to_string().c_str());

    auto packet = tcp.outgoing_packet();
    packet->set_seq(tcb.ISS).set_ack(tcb.RCV.NXT).set_flags(SYN | ACK);

    /*
      Add MSS option.
      TODO: Send even if we havent received MSS option?
    */
    tcp.add_option(Option::MSS, packet);

    tcp.transmit(packet);
    tcp.set_state(SynReceived::instance());

    return OK;
  }
  return OK;
}


State::Result Connection::SynSent::handle(Connection& tcp, Packet_ptr in) {
  // 1. check ACK
  if(in->isset(ACK)) {
    auto& tcb = tcp.tcb();
    //  If SEG.ACK =< ISS, or SEG.ACK > SND.NXT
    if(in->ack() <= tcb.ISS or in->ack() > tcb.SND.NXT) {
      // send a reset
      if(!in->isset(RST)) {
        auto packet = tcp.outgoing_packet();
        packet->set_seq(in->ack()).set_flag(RST);
        tcp.transmit(packet);
        return OK;
      }
      // (unless the RST bit is set, if so drop the segment and return)
      else {
        tcp.drop(in, "RST");
        return OK;
      }
      // If SND.UNA =< SEG.ACK =< SND.NXT then the ACK is acceptable.
    } else {
      if(tcp.rttm.active)
        tcp.rttm.stop(true);
    }
  }

  // 2. check RST
  if(in->isset(RST)) {
    if(in->isset(ACK)) {
      tcp.signal_error(TCPException{"Connection reset."});
      tcp.drop(in, "RST with acceptable ACK");
      return CLOSED;
    } else {
      tcp.drop(in, "RST");
      return OK;
    }
    /*
      If the ACK was acceptable then signal the user "error:
      connection reset", drop the segment, enter CLOSED state,
      delete TCB, and return.  Otherwise (no ACK) drop the segment
      and return.
    */
  }
  // 3. Check security

  // 4. check SYN
  /*
    This step should be reached only if the ACK is ok, or there is
    no ACK, and it the segment did not contain a RST.

    If the SYN bit is on and the security/compartment and precedence
    are acceptable then, RCV.NXT is set to SEG.SEQ+1, IRS is set to
    SEG.SEQ.  SND.UNA should be advanced to equal SEG.ACK (if there
    is an ACK), and any segments on the retransmission queue which
    are thereby acknowledged should be removed.

    If SND.UNA > ISS (our SYN has been ACKed), change the connection
    state to ESTABLISHED, form an ACK segment

    <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK>

    and send it.  Data or controls which were queued for
    transmission may be included.  If there are other controls or
    text in the segment then continue processing at the sixth step
    below where the URG bit is checked, otherwise return.

    Otherwise enter SYN-RECEIVED, form a SYN,ACK segment

    <SEQ=ISS><ACK=RCV.NXT><CTL=SYN,ACK>

    and send it.  If there are other controls or text in the
    segment, queue them for processing after the ESTABLISHED state
    has been reached, return.
  */

  // TODO: Fix this one according to the text above.
  if(in->isset(SYN)) {
    auto& tcb = tcp.tcb();
    tcb.RCV.NXT   = in->seq()+1;
    tcb.IRS       = in->seq();
    tcb.SND.UNA   = in->ack();

    if(tcp.rtx_timer.active)
      tcp.rtx_stop();

    // (our SYN has been ACKed)
    if(tcb.SND.UNA > tcb.ISS) {
      tcp.set_state(Connection::Established::instance());
      // Correction: [RFC 1122 p. 94]
      tcb.SND.WND = in->win();
      tcb.SND.WL1 = in->seq();
      tcb.SND.WL2 = in->ack();
      // end of correction

      seq_t snd_nxt = tcb.SND.NXT;
      tcp.signal_connect(); // NOTE: User callback

      if(tcb.SND.NXT == snd_nxt) {
        auto packet = tcp.outgoing_packet();
        packet->set_seq(tcb.SND.NXT).set_ack(tcb.RCV.NXT).set_flag(ACK);
        tcp.transmit(packet);
      }
      // State is now ESTABLISHED.
      // Experimental, also makes unessecary process.
      //in->clear_flag(SYN);
      //tcp.state().handle(tcp, in);

      // 7. process segment text
      if(in->has_tcp_data()) {
        process_segment(tcp, in);
      }

      // 8. check FIN bit
      if(in->isset(FIN)) {
        process_fin(tcp, in);
        tcp.set_state(Connection::CloseWait::instance());
        return OK;
      }
      return OK;
    }
    // Otherwise enter SYN-RECEIVED, form a SYN,ACK segment <SEQ=ISS><ACK=RCV.NXT><CTL=SYN,ACK>
    else {
      auto packet = tcp.outgoing_packet();
      packet->set_seq(tcb.ISS).set_ack(tcb.RCV.NXT).set_flags(SYN | ACK);
      tcp.transmit(packet);
      tcp.set_state(Connection::SynReceived::instance());
      if(in->has_tcp_data()) {
        process_segment(tcp, in);
      }
      return OK;
      /*
        If there are other controls or text in the
        segment, queue them for processing after the ESTABLISHED state
        has been reached, return.

        HOW? return tcp.receive(in); ?
      */
    }
  }
  tcp.drop(in);
  return OK;
}


State::Result Connection::SynReceived::handle(Connection& tcp, Packet_ptr in) {
  // 1. check sequence
  if(! check_seq(tcp, in) ) {
    return OK;
  }
  // 2. check RST
  if(in->isset(RST)) {
    /*
      If this connection was initiated with a passive OPEN (i.e.,
      came from the LISTEN state), then return this connection to
      LISTEN state and return.  The user need not be informed.  If
      this connection was initiated with an active OPEN (i.e., came
      from SYN-SENT state) then the connection was refused, signal
      the user "connection refused".  In either case, all segments
      on the retransmission queue should be removed.  And in the
      active OPEN case, enter the CLOSED state and delete the TCB,
      and return.
    */
    // Since we create a new connection when it starts listening, we don't wanna do this, but just delete it.
    // TODO: Remove string comparision
    if(tcp.prev_state().to_string() == Connection::SynSent::instance().to_string()) {
      tcp.signal_disconnect(Disconnect::REFUSED);
    }

    return CLOSED;
  }
  // 3. check security

  // 4. check SYN
  if( in->isset(SYN) ) {
    unallowed_syn_reset_connection(tcp, in);
    return CLOSED;
  }

  // 5. check ACK
  if(in->isset(ACK)) {
    auto& tcb = tcp.tcb();
    /*
      If SND.UNA =< SEG.ACK =< SND.NXT then enter ESTABLISHED state
      and continue processing.
    */
    if(tcb.SND.UNA <= in->ack() and in->ack() <= tcb.SND.NXT) {
      debug("<Connection::SynReceived::handle> SND.UNA =< SEG.ACK =< SND.NXT, continue in ESTABLISHED. \n");
      if(tcp.rttm.active)
        tcp.rttm.stop(true);
      tcp.set_state(Connection::Established::instance());

      // Taken from acknowledge (without congestion control)
      tcb.SND.UNA = in->ack();
      if(tcp.rtx_timer.active)
        tcp.rtx_stop();

      tcp.signal_connect(); // NOTE: User callback

      // 7. proccess the segment text
      if(in->has_tcp_data()) {
        debug2("<Connection::SynReceived::handle> @warning: Packet has data? %s\n", in->to_string().c_str());
        process_segment(tcp, in);
      }

      // 8. check FIN bit
      if(in->isset(FIN)) {
        tcp.set_state(Connection::CloseWait::instance());
        process_fin(tcp, in);
        return OK;
      }
    }
    /*
      If the segment acknowledgment is not acceptable, form a
      reset segment, <SEQ=SEG.ACK><CTL=RST> and send it.
    */
    else {
      auto packet = tcp.outgoing_packet();
      packet->set_seq(in->ack()).set_flag(RST);
      tcp.transmit(packet);
    }
  }
  // ACK is missing
  else {
    tcp.drop(in, "SYN-RCV: !ACK");
    return OK;
  }

  // 8. check FIN
  if(in->isset(FIN)) {
    process_fin(tcp, in);
    tcp.set_state(Connection::CloseWait::instance());
    return OK;
  }
  return OK;
}


State::Result Connection::Established::handle(Connection& tcp, Packet_ptr in) {
  // 1. check SEQ
  if(! check_seq(tcp, in) ) {
    return OK;
  }

  // 2. check RST
  if( in->isset(RST) ) {
    tcp.signal_disconnect(Disconnect::RESET);
    return CLOSED; // close
  }

  // 3. check security

  // 4. check SYN
  if( in->isset(SYN) ) {
    unallowed_syn_reset_connection(tcp, in);
    return CLOSED;
  }

  // 5. check ACK
  if( ! check_ack(tcp, in) ) {
    return OK;
  }
  // 6. check URG - DEPRECATED

  // 7. proccess the segment text
  if(in->has_tcp_data()) {
    process_segment(tcp, in);
  }

  // 8. check FIN bit
  if(in->isset(FIN)) {
    tcp.set_state(Connection::CloseWait::instance());
    process_fin(tcp, in);
    return OK;
  }

  return OK;
}


State::Result Connection::FinWait1::handle(Connection& tcp, Packet_ptr in) {
  // 1. Check sequence number
  if(! check_seq(tcp, in) ) {
    return OK;
  }

  // 2. check RST
  if( in->isset(RST) ) {
    tcp.signal_disconnect(Disconnect::RESET);
    return CLOSED; // close
  }

  // 4. check SYN
  if( in->isset(SYN) ) {
    unallowed_syn_reset_connection(tcp, in);
    return CLOSED;
  }

  // 5. check ACK
  if( ! check_ack(tcp, in) ) {
    return OK;
  }
  /*
    In addition to the processing for the ESTABLISHED state, if
    our FIN is now acknowledged then enter FIN-WAIT-2 and continue
    processing in that state.
  */
  debug2("<Connection::FinWait1::handle> Current TCB:\n %s \n", tcp.tcb().to_string().c_str());
  if(in->ack() == tcp.tcb().SND.NXT) {
    // TODO: I guess or FIN is ACK'ed..?
    tcp.set_state(Connection::FinWait2::instance());
    return tcp.state_->handle(tcp, in); // TODO: Is this OK?
  }

  // 7. proccess the segment text
  if(in->has_tcp_data()) {
    process_segment(tcp, in);
  }

  // 8. check FIN
  if(in->isset(FIN)) {
    process_fin(tcp, in);
    debug2("<Connection::FinWait1::handle> FIN isset. TCB:\n %s \n", tcp.tcb().to_string().c_str());
    /*
      If our FIN has been ACKed (perhaps in this segment), then
      enter TIME-WAIT, start the time-wait timer, turn off the other
      timers; otherwise enter the CLOSING state.
    */
    if(in->ack() == tcp.tcb().SND.NXT) {
      // TODO: I guess or FIN is ACK'ed..?
      tcp.set_state(TimeWait::instance());
      if(tcp.rtx_timer.active)
        tcp.rtx_stop();
      tcp.timewait_start();
    } else {
      tcp.set_state(Closing::instance());
    }
  }
  return OK;
}


State::Result Connection::FinWait2::handle(Connection& tcp, Packet_ptr in) {
  // 1. check SEQ
  if(! check_seq(tcp, in) ) {
    return OK;
  }

  // 2. check RST
  if( in->isset(RST) ) {
    tcp.signal_disconnect(Disconnect::RESET);
    return CLOSED; // close
  }

  // 4. check SYN
  if( in->isset(SYN) ) {
    unallowed_syn_reset_connection(tcp, in);
    return CLOSED;
  }

  // 5. check ACK
  if( ! check_ack(tcp, in) ) {
    return OK;
  }

  // 7. proccess the segment text
  if(in->has_tcp_data()) {
    process_segment(tcp, in);
  }

  // 8. check FIN
  if(in->isset(FIN)) {
    process_fin(tcp, in);
    /*
      Enter the TIME-WAIT state.
      Start the time-wait timer, turn off the other timers.
    */
    tcp.set_state(Connection::TimeWait::instance());
    if(tcp.rtx_timer.active)
      tcp.rtx_stop();
    tcp.timewait_start();
  }
  return OK;
}


State::Result Connection::CloseWait::handle(Connection& tcp, Packet_ptr in) {
  // 1. check SEQ
  if(! check_seq(tcp, in) ) {
    return OK;
  }

  // 2. check RST
  if( in->isset(RST) ) {
    tcp.signal_disconnect(Disconnect::RESET);
    return CLOSED; // close
  }

  // 4. check SYN
  if( in->isset(SYN) ) {
    unallowed_syn_reset_connection(tcp, in);
    return CLOSED;
  }

  // 5. check ACK
  if( ! check_ack(tcp, in) ) {
    return OK;
  }

  // 7. proccess the segment text
  // This should not occur, since a FIN has been received from the remote side.  Ignore the segment text.

  // 8. check FIN
  if(in->isset(FIN)) {
    process_fin(tcp, in);
    // Remain in state
    return OK;
  }
  return OK;
}


State::Result Connection::Closing::handle(Connection& tcp, Packet_ptr in) {
  // 1. Check sequence number
  if(! check_seq(tcp, in) ) {
    return OK;
  }

  // 2. check RST
  if( in->isset(RST) ) {
    return CLOSED; // close
  }

  // 4. check SYN
  if( in->isset(SYN) ) {
    unallowed_syn_reset_connection(tcp, in);
    return CLOSED;
  }

  // 5. check ACK
  if( ! check_ack(tcp, in)) {
    return CLOSED;
  }

  /*
    In addition to the processing for the ESTABLISHED state, if
    the ACK acknowledges our FIN then enter the TIME-WAIT state,
    otherwise ignore the segment.
  */
  if(in->ack() == tcp.tcb().SND.NXT) {
    // TODO: I guess or FIN is ACK'ed..?
    tcp.set_state(TimeWait::instance());
    tcp.timewait_start();
  }

  // 7. proccess the segment text
  // This should not occur, since a FIN has been received from the remote side.  Ignore the segment text.

  // 8. check FIN
  if(in->isset(FIN)) {
    process_fin(tcp, in);
    // Remain in state
    return OK;
  }
  return OK;
}


State::Result Connection::LastAck::handle(Connection& tcp, Packet_ptr in) {
  // 1. Check sequence number
  if(! check_seq(tcp, in) ) {
    return OK;
  }
  return CLOSED;

  // 2. check RST
  if( in->isset(RST) ) {
    return CLOSED; // close
  }

  // 4. check SYN
  if( in->isset(SYN) ) {
    unallowed_syn_reset_connection(tcp, in);
    return CLOSED;
  }

  if( ! check_ack(tcp, in)) {
    return CLOSED;
  }

  // 7. proccess the segment text
  // This should not occur, since a FIN has been received from the remote side.  Ignore the segment text.

  // 8. check FIN
  if(in->isset(FIN)) {
    process_fin(tcp, in);
    // Remain in state
    return OK;
  }
  return OK;
}


State::Result Connection::TimeWait::handle(Connection& tcp, Packet_ptr in) {
  // 1. Check sequence number
  if(! check_seq(tcp, in) ) {
    return OK;
  }

  // 2. check RST
  if( in->isset(RST) ) {
    return CLOSED; // close
  }

  // 4. check SYN
  if( in->isset(SYN) ) {
    unallowed_syn_reset_connection(tcp, in);
    return CLOSED;
  }

  // 7. proccess the segment text
  // This should not occur, since a FIN has been received from the remote side.  Ignore the segment text.

  // 8. check FIN
  if(in->isset(FIN)) {
    process_fin(tcp, in);
    // Remain in state
    tcp.timewait_restart();
    return OK;
  }
  return OK;
}
/////////////////////////////////////////////////////////////////////
