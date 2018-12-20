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

#include <net/tcp/connection_states.hpp>

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
bool Connection::State::check_seq(Connection& tcp, Packet_view& in)
{
  auto& tcb = tcp.tcb();

  // RFC 7323
  static constexpr uint8_t HEADER_WITH_TS{sizeof(Header) + 12};
  if(tcb.SND.TS_OK and in.tcp_header_length() == HEADER_WITH_TS)
  {
    const auto* ts = in.parse_ts_option();
    in.set_ts_option(ts);

    // PAWS
    if(UNLIKELY(ts != nullptr and (ts->get_val() < tcb.TS_recent and !in.isset(RST))))
    {
      /*
        If the connection has been idle more than 24 days,
        save SEG.TSval in variable TS.Recent, else the segment
        is not acceptable; follow the steps below for an
        unacceptable segment.
      */
      goto unacceptable;
    }
  }

  debug2("<Connection::State::check_seq> TCB: %s \n",tcb.to_string().c_str());
  // #1 - The packet we expect
  if( in.seq() == tcb.RCV.NXT )
  {
    goto acceptable;
  }
  /// if SACK isn't permitted there is no point handling out-of-order packets
  else if(not tcp.sack_perm)
    goto unacceptable;

  // #2 - Packet is ahead of what we expect to receive, but inside our window
  if( (in.seq() - tcb.RCV.NXT) < tcb.RCV.WND ) {
    goto acceptable;
  }
  /*
    If an incoming segment is not acceptable, an acknowledgment
    should be sent in reply (unless the RST bit is set, if so drop
    the segment and return):

    <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK>

    After sending the acknowledgment, drop the unacceptable segment
    and return.
  */

unacceptable:
  tcp.update_rcv_wnd();

  if(!in.isset(RST))
    tcp.send_ack();

  tcp.drop(in, Drop_reason::SEQ_OUT_OF_ORDER);
  return false;

acceptable:
  const auto* ts = in.ts_option();
  if(tcb.SND.TS_OK)
    ts = in.parse_ts_option();

  if(ts != nullptr and
    (ts->get_val() >= tcb.TS_recent and in.seq() <= tcp.last_ack_sent_))
  {
    tcb.TS_recent = ts->get_val();
  }
  debug2("<Connection::State::check_seq> Acceptable SEQ: %u \n", in.seq());
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

void Connection::State::unallowed_syn_reset_connection(Connection& tcp, const Packet_view& in) {
  assert(in.isset(SYN));
  debug("<Connection::State::unallowed_syn_reset_connection> Unallowed SYN for STATE: %s, reseting connection. %s\n",
        tcp.state().to_string().c_str(), in.to_string().c_str());
  // Not sure if this is the correct way to send a "reset response"
  auto packet = tcp.outgoing_packet();
  packet->set_seq(in.ack()).set_flag(RST);
  tcp.transmit(std::move(packet));
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

bool Connection::State::check_ack(Connection& tcp, const Packet_view& in) {
  debug2("<Connection::State::check_ack> Checking for ACK in STATE: %s \n", tcp.state().to_string().c_str());
  if( in.isset(ACK) ) {
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
    //return tcp.handle_ack(in);

    if ( (in.ack()-tcb.SND.UNA) <= (tcb.SND.NXT-tcb.SND.UNA)) {

      return tcp.handle_ack(in);
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

    }
    /* If the ACK acks something not yet sent (SEG.ACK > SND.NXT) then send an ACK, drop the segment, and return. */
    else {
      auto packet = tcp.outgoing_packet();
      packet->set_flag(ACK);
      tcp.transmit(std::move(packet));
      tcp.drop(in, Drop_reason::ACK_OUT_OF_ORDER);
      return false;
    }
    return true;
  }
  // ACK not set.
  else {
    tcp.drop(in, Drop_reason::ACK_NOT_SET);
    return false;
  }
}

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

void Connection::State::process_fin(Connection& tcp, const Packet_view& in) {
  debug2("<Connection::State::process_fin> Processing FIN bit in STATE: %s \n", tcp.state().to_string().c_str());
  Expects(in.isset(FIN));
  auto& tcb = tcp.tcb();
  // Advance RCV.NXT over the FIN?
  tcb.RCV.NXT++;
  //auto fin = tcp_data_length();
  //tcb.RCV.NXT += fin;
  const auto snd_nxt = tcb.SND.NXT;
  // empty the read buffer
  if(tcp.read_request and tcp.read_request->size())
    tcp.receive_disconnect();
  // signal disconnect to the user
  tcp.signal_disconnect(Disconnect::CLOSING);

  // only ack FIN if user callback didn't result in a sent packet
  if(tcb.SND.NXT == snd_nxt) {
    debug2("<Connection::State::process_fin> acking FIN\n");
    auto packet = tcp.outgoing_packet();
    packet->set_ack(tcb.RCV.NXT).set_flag(ACK);
    tcp.transmit(std::move(packet));
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
  tcp.transmit(std::move(packet));
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
      tcp.add_option(Option::MSS, *packet);

      // Window scaling
      if(tcp.uses_window_scaling())
      {
        tcp.add_option(Option::WS, *packet);
        packet->set_win(std::min((uint32_t)default_window_size, tcb.RCV.WND));
      }
      // Add timestamps
      if(tcp.uses_timestamps())
      {
        tcp.add_option(Option::TS, *packet);
      }

      if(tcp.uses_SACK())
      {
        tcp.add_option(Option::SACK_PERM, *packet);
      }

      tcb.SND.UNA = tcb.ISS;
      tcb.SND.NXT = tcb.ISS+1;
      tcp.transmit(std::move(packet));
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
    tcp.transmit(std::move(packet));
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

size_t Connection::Established::send(Connection&, WriteBuffer&) {

  return 0;
}

size_t Connection::CloseWait::send(Connection&, WriteBuffer&) {

  return 0;
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
  if(not tcp.writeq.has_remaining_requests())
  {
    auto& tcb = tcp.tcb();
    auto packet = tcp.outgoing_packet();
    packet->set_seq(tcb.SND.NXT++).set_ack(tcb.RCV.NXT).set_flags(ACK | FIN);
    tcp.transmit(std::move(packet));
  }

  tcp.set_state(Connection::FinWait1::instance());
}

void Connection::Established::close(Connection& tcp) {
  if(not tcp.writeq.has_remaining_requests())
  {
    auto& tcb = tcp.tcb();
    auto packet = tcp.outgoing_packet();
    packet->set_seq(tcb.SND.NXT++).set_ack(tcb.RCV.NXT).set_flags(ACK | FIN);
    tcp.transmit(std::move(packet));
  }

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
  if(not tcp.writeq.has_remaining_requests()) {
    auto& tcb = tcp.tcb();
    auto packet = tcp.outgoing_packet();
    packet->set_seq(tcb.SND.NXT++).set_ack(tcb.RCV.NXT).set_flags(ACK | FIN);
    tcp.transmit(std::move(packet));
  }
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

State::Result Connection::Closed::handle(Connection& tcp, Packet_view& in) {
  if(in.isset(RST)) {
    return OK;
  }
  auto packet = tcp.outgoing_packet();
  if(!in.isset(ACK)) {
    packet->set_seq(0).set_ack(in.seq() + in.tcp_data_length()).set_flags(RST | ACK);
  } else {
    packet->set_seq(in.ack()).set_flag(RST);
  }
  tcp.transmit(std::move(packet));
  return OK;
}


State::Result Connection::Listen::handle(Connection& tcp, Packet_view& in) {
  if(UNLIKELY(in.isset(RST)))
    return OK;

  if(UNLIKELY(in.isset(ACK)))
  {
    auto packet = tcp.outgoing_packet();
    packet->set_seq(in.ack()).set_flag(RST);
    tcp.transmit(std::move(packet));
    return OK;
  }

  if(in.isset(SYN))
  {
    auto& tcb = tcp.tcb();
    tcb.RCV.NXT   = in.seq()+1;
    tcb.IRS       = in.seq();
    tcb.init();
    tcb.SND.NXT   = tcb.ISS+1;
    tcb.SND.UNA   = tcb.ISS;
    debug("<Connection::Listen::handle> Received SYN Packet: %s TCB Updated:\n %s \n",
          in.to_string().c_str(), tcp.tcb().to_string().c_str());

    // Parse options
    tcp.parse_options(in);

    auto packet = tcp.outgoing_packet();
    packet->set_seq(tcb.ISS).set_ack(tcb.RCV.NXT).set_flags(SYN | ACK);

    /*
      Add MSS option.
      TODO: Send even if we havent received MSS option?
    */
    tcp.add_option(Option::MSS, *packet);

    // This means WS was accepted in the SYN packet
    if(tcb.SND.wind_shift > 0)
    {
      tcp.add_option(Option::WS, *packet);
      packet->set_win(std::min((uint32_t)default_window_size, tcb.RCV.WND));
    }

    // SACK permitted
    if(tcp.sack_perm == true)
    {
      tcp.add_option(Option::SACK_PERM, *packet);
    }

    tcp.transmit(std::move(packet));
    tcp.set_state(SynReceived::instance());

    return OK;
  }
  return OK;
}


State::Result Connection::SynSent::handle(Connection& tcp, Packet_view& in) {
  // 1. check ACK
  if(in.isset(ACK)) {
    auto& tcb = tcp.tcb();
    //  If SEG.ACK =< ISS, or SEG.ACK > SND.NXT
    if(UNLIKELY(in.ack() <= tcb.ISS or in.ack() > tcb.SND.NXT))
    {
      // send a reset
      if(!in.isset(RST)) {
        auto packet = tcp.outgoing_packet();
        packet->set_seq(in.ack()).set_flag(RST);
        tcp.transmit(std::move(packet));
        return OK;
      }
      // (unless the RST bit is set, if so drop the segment and return)
      else {
        tcp.drop(in, Drop_reason::RST);
        return OK;
      }
      // If SND.UNA =< SEG.ACK =< SND.NXT then the ACK is acceptable.
    }
  }

  // 2. check RST
  if(UNLIKELY(in.isset(RST))) {
    if(in.isset(ACK)) {
      tcp.signal_connect(false);
      tcp.drop(in, Drop_reason::RST);
      return CLOSED;
    } else {
      tcp.drop(in, Drop_reason::RST);
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
  if(in.isset(SYN)) {
    auto& tcb = tcp.tcb();
    tcb.RCV.NXT   = in.seq()+1;
    tcb.IRS       = in.seq();
    tcb.SND.UNA   = in.ack();

    if(tcp.rtx_timer.is_running())
      tcp.rtx_stop();

    // Parse options
    tcp.parse_options(in);

    tcp.take_rtt_measure(in);

    // (our SYN has been ACKed)
    if(tcb.SND.UNA > tcb.ISS)
    {
      // Correction: [RFC 1122 p. 94]
      tcb.SND.WND = in.win();
      tcb.SND.WL1 = in.seq();
      tcb.SND.WL2 = in.ack();
      // end of correction

      // [RFC 6298] p.4 (5.7)
      if(UNLIKELY(tcp.syn_rtx_ > 0))
      {
        tcp.syn_rtx_ = 0;
        tcp.rttm.RTO = RTTM::seconds(3.0);
      }

      // make sure to send an ACK to fullfil the handshake
      // before calling user callback in case of user write
      tcp.send_ack();

      tcp.set_state(Connection::Established::instance());

      tcp.signal_connect(); // NOTE: User callback

      if(tcp.has_doable_job())
        tcp.writeq_push();

      // 7. process segment text
      if(UNLIKELY(in.has_tcp_data()))
      {
        tcp.recv_data(in);
      }

      // 8. check FIN bit
      if(UNLIKELY(in.isset(FIN)))
      {
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
      tcp.transmit(std::move(packet));
      tcp.set_state(Connection::SynReceived::instance());
      if(in.has_tcp_data()) {
        tcp.recv_data(in);
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


State::Result Connection::SynReceived::handle(Connection& tcp, Packet_view& in) {
  // 1. check sequence
  if(UNLIKELY(! check_seq(tcp, in) )) {
    return OK;
  }
  // 2. check RST
  if(UNLIKELY(in.isset(RST))) {
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
    if(&tcp.prev_state() == &Connection::SynSent::instance()) {
      tcp.signal_disconnect(Disconnect::REFUSED);
    }

    return CLOSED;
  }
  // 3. check security

  // 4. check SYN
  if(UNLIKELY(in.isset(SYN)))
  {
    unallowed_syn_reset_connection(tcp, in);
    return CLOSED;
  }

  // 5. check ACK
  if(in.isset(ACK)) {
    auto& tcb = tcp.tcb();
    /*
      If SND.UNA =< SEG.ACK =< SND.NXT then enter ESTABLISHED state
      and continue processing.
    */
    if(tcb.SND.UNA <= in.ack() and in.ack() <= tcb.SND.NXT)
    {
      debug2("<Connection::SynReceived::handle> %s SND.UNA =< SEG.ACK =< SND.NXT, continue in ESTABLISHED.\n",
        tcp.to_string().c_str());

      tcp.set_state(Connection::Established::instance());

      tcp.handle_ack(in);

      // [RFC 6298] p.4 (5.7)
      if(UNLIKELY(tcp.syn_rtx_ > 0))
      {
        tcp.syn_rtx_ = 0;
        tcp.rttm.RTO = RTTM::seconds(3.0);
      }

      tcp.signal_connect(); // NOTE: User callback

      // 7. proccess the segment text
      if(UNLIKELY(in.has_tcp_data())) {
        tcp.recv_data(in);
      }
    }
    /*
      If the segment acknowledgment is not acceptable, form a
      reset segment, <SEQ=SEG.ACK><CTL=RST> and send it.
    */
    else {
      auto packet = tcp.outgoing_packet();
      packet->set_seq(in.ack()).set_flag(RST);
      tcp.transmit(std::move(packet));
    }
  }
  // ACK is missing
  else {
    tcp.drop(in, Drop_reason::ACK_NOT_SET);
    return OK;
  }

  // 8. check FIN
  if(UNLIKELY(in.isset(FIN))) {
    process_fin(tcp, in);
    tcp.set_state(Connection::CloseWait::instance());
    return OK;
  }
  return OK;
}


State::Result Connection::Established::handle(Connection& tcp, Packet_view& in) {
  // 1. check SEQ
  if(UNLIKELY(! check_seq(tcp, in) )) {
    return OK;
  }

  // 2. check RST
  if(UNLIKELY( in.isset(RST) )) {
    tcp.signal_disconnect(Disconnect::RESET);
    return CLOSED; // close
  }

  // 3. check security

  // 4. check SYN
  if(UNLIKELY( in.isset(SYN) )) {
    unallowed_syn_reset_connection(tcp, in);
    return CLOSED;
  }

  // 5. check ACK
  if(UNLIKELY( ! check_ack(tcp, in) )) {
    return OK;
  }
  // 6. check URG - DEPRECATED

  // 7. proccess the segment text
  if(in.has_tcp_data()) {
    tcp.recv_data(in);
  }

  // 8. check FIN bit
  if(UNLIKELY(in.isset(FIN))) {
    tcp.set_state(Connection::CloseWait::instance());
    process_fin(tcp, in);
    return OK;
  }

  return OK;
}


State::Result Connection::FinWait1::handle(Connection& tcp, Packet_view& in) {
  // 1. Check sequence number
  if(UNLIKELY(! check_seq(tcp, in) )) {
    return OK;
  }

  // 2. check RST
  if(UNLIKELY( in.isset(RST) )) {
    tcp.signal_disconnect(Disconnect::RESET);
    return CLOSED; // close
  }

  // 4. check SYN
  if(UNLIKELY( in.isset(SYN) )) {
    unallowed_syn_reset_connection(tcp, in);
    return CLOSED;
  }

  // 5. check ACK
  if(UNLIKELY( ! check_ack(tcp, in) )) {
    return OK;
  }
  /*
    In addition to the processing for the ESTABLISHED state, if
    our FIN is now acknowledged then enter FIN-WAIT-2 and continue
    processing in that state.
  */
  debug2("<Connection::FinWait1::handle> Current TCB:\n %s \n", tcp.tcb().to_string().c_str());
  if(in.ack() == tcp.tcb().SND.NXT) {
    // TODO: I guess or FIN is ACK'ed..?
    tcp.set_state(Connection::FinWait2::instance());
    return tcp.state_->handle(tcp, in); // TODO: Is this OK?
  }

  // 7. proccess the segment text
  if(in.has_tcp_data()) {
    tcp.recv_data(in);
  }

  // 8. check FIN
  if(in.isset(FIN)) {
    process_fin(tcp, in);
    debug2("<Connection::FinWait1::handle> FIN isset. TCB:\n %s \n", tcp.tcb().to_string().c_str());
    /*
      If our FIN has been ACKed (perhaps in this segment), then
      enter TIME-WAIT, start the time-wait timer, turn off the other
      timers; otherwise enter the CLOSING state.
    */
    if(in.ack() == tcp.tcb().SND.NXT) {
      // TODO: I guess or FIN is ACK'ed..?
      tcp.set_state(TimeWait::instance());
      tcp.release_memory();
      if(tcp.rtx_timer.is_running())
        tcp.rtx_stop();
      tcp.timewait_start();
    } else {
      tcp.set_state(Closing::instance());
    }
  }
  return OK;
}


State::Result Connection::FinWait2::handle(Connection& tcp, Packet_view& in) {
  // 1. check SEQ
  if(! check_seq(tcp, in) ) {
    return OK;
  }

  // 2. check RST
  if( in.isset(RST) ) {
    tcp.signal_disconnect(Disconnect::RESET);
    return CLOSED; // close
  }

  // 4. check SYN
  if( in.isset(SYN) ) {
    unallowed_syn_reset_connection(tcp, in);
    return CLOSED;
  }

  // 5. check ACK
  if( ! check_ack(tcp, in) ) {
    return OK;
  }

  // 7. proccess the segment text
  if(in.has_tcp_data()) {
    tcp.recv_data(in);
  }

  // 8. check FIN
  if(in.isset(FIN)) {
    process_fin(tcp, in);
    /*
      Enter the TIME-WAIT state.
      Start the time-wait timer, turn off the other timers.
    */
    tcp.set_state(Connection::TimeWait::instance());
    tcp.release_memory();
    if(tcp.rtx_timer.is_running())
      tcp.rtx_stop();
    tcp.timewait_start();
  }
  return OK;
}


State::Result Connection::CloseWait::handle(Connection& tcp, Packet_view& in) {
  // 1. check SEQ
  if(! check_seq(tcp, in) ) {
    return OK;
  }

  // 2. check RST
  if( in.isset(RST) ) {
    tcp.signal_disconnect(Disconnect::RESET);
    return CLOSED; // close
  }

  // 4. check SYN
  if( in.isset(SYN) ) {
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
  if(in.isset(FIN)) {
    process_fin(tcp, in);
    // Remain in state
    return OK;
  }
  return OK;
}


State::Result Connection::Closing::handle(Connection& tcp, Packet_view& in) {
  // 1. Check sequence number
  if(! check_seq(tcp, in) ) {
    return OK;
  }

  // 2. check RST
  if( in.isset(RST) ) {
    return CLOSED; // close
  }

  // 4. check SYN
  if( in.isset(SYN) ) {
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
  if(in.ack() == tcp.tcb().SND.NXT) {
    // TODO: I guess or FIN is ACK'ed..?
    tcp.set_state(TimeWait::instance());
    tcp.release_memory();
    tcp.timewait_start();
  }

  // 7. proccess the segment text
  // This should not occur, since a FIN has been received from the remote side.  Ignore the segment text.

  // 8. check FIN
  if(in.isset(FIN)) {
    process_fin(tcp, in);
    // Remain in state
    return OK;
  }
  return OK;
}


State::Result Connection::LastAck::handle(Connection& tcp, Packet_view& in) {
  // 1. Check sequence number
  if(! check_seq(tcp, in) ) {
    return OK;
  }
  return CLOSED;

  // 2. check RST
  if( in.isset(RST) ) {
    return CLOSED; // close
  }

  // 4. check SYN
  if( in.isset(SYN) ) {
    unallowed_syn_reset_connection(tcp, in);
    return CLOSED;
  }

  if( ! check_ack(tcp, in)) {
    return CLOSED;
  }

  // 7. proccess the segment text
  // This should not occur, since a FIN has been received from the remote side.  Ignore the segment text.

  // 8. check FIN
  if(in.isset(FIN)) {
    process_fin(tcp, in);
    // Remain in state
    return OK;
  }
  return OK;
}


State::Result Connection::TimeWait::handle(Connection& tcp, Packet_view& in) {
  // 1. Check sequence number
  if(! check_seq(tcp, in) ) {
    return OK;
  }

  // 2. check RST
  if( in.isset(RST) ) {
    return CLOSED; // close
  }

  // 4. check SYN
  if( in.isset(SYN) ) {
    unallowed_syn_reset_connection(tcp, in);
    return CLOSED;
  }

  // 7. proccess the segment text
  // This should not occur, since a FIN has been received from the remote side.  Ignore the segment text.

  // 8. check FIN
  if(in.isset(FIN)) {
    process_fin(tcp, in);
    // Remain in state
    tcp.timewait_restart();
    return OK;
  }
  return OK;
}
/////////////////////////////////////////////////////////////////////
