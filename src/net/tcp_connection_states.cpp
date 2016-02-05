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

#include <net/tcp_connection_states.hpp>
#include <algorithm>

using namespace std;

/*
	// #1
	(in->seq() == tcb.RCV.NXT);
	// #2
    (tcb.RCV.NXT <= in->seq() and in->seq() < tcb.RCV.NXT + tcb.RCV.WND);
    // #3 invalid
    (in->seq() > tcb.RCV.WND);
    // #4
    (tcb.RCV.NXT <= in->seq() and in->seq() < tcb.RCV.NXT + tcb.RCV.WND)
    	or (tcb.RCV.NXT <= in->seq()+in->data_length()-1 and in->seq()+in->data_length()-1 < tcb.RCV.NXT+tcb.RCV.WND);
*/
bool acceptable(Connection& tcp, TCP::Packet_ptr in) {
	auto& tcb = tcp.tcb();
	if( in->seq() == tcb.RCV.NXT ) {
		return true;
	} 
	else if( tcb.RCV.NXT <= in->seq() and in->seq() < tcb.RCV.NXT + tcb.RCV.WND ) {
		return true;
	}
	else if( in->seq() > tcb.RCV.WND ) {
		return false;
	} else if( (tcb.RCV.NXT <= in->seq() and in->seq() < tcb.RCV.NXT + tcb.RCV.WND)
    	or ( tcb.RCV.NXT <= in->seq()+in->data_length()-1 and in->seq()+in->data_length()-1 < tcb.RCV.NXT+tcb.RCV.WND ) ) {
		return true;
	} else {
		return false;
	}
}



// STATE - Fallback if not implemented in state.

void Connection::State::open(Connection& tcp, bool active) {
	throw TCPException{"Connection already exists."};
}

void Connection::State::send(Connection& tcp, const std::string& data, bool push) {
	throw TCPException{"Connection is not open."};
}

void Connection::State::receive(Connection& tcp, size_t buffer_size) {

}

void Connection::State::close(Connection& tcp) {
	// Dirty close
}

int Connection::State::handle(Connection& tcp, TCP::Packet_ptr in) {
	tcp.drop(in);
}

string Connection::State::to_string() const {
	return "STATELESS";
}

/*
	CLOSED
*/

void Connection::Closed::open(Connection& tcp, bool active) {
	if(active) {
		// There is a remote host
		if(!tcp.remote().is_empty()) {
			auto out = tcp.outgoing_packet();
			auto& tcb = tcp.tcb();
			tcb.ISS = tcp.generate_iss();
			out->set_seq(tcb.ISS).set_flag(SYN);
			tcb.SND.UNA = tcb.ISS;
			tcb.SND.NXT = tcb.ISS+1;
			tcp.transmit();
			tcp.set_state(SynSent::instance());	
		} else {
			throw TCPException{"No remote host set."};
		}
	} else {
		tcp.set_state(Listen::instance());
	}
}

int Connection::Closed::handle(Connection& tcp, TCP::Packet_ptr in) {
	if(in->isset(RST)) {
		return 0;
	}
	auto out = tcp.outgoing_packet();
	if(!in->isset(ACK)) {
		out->set_seq(0).set_ack(in->seq() + in->data_length()).set_flags(RST | ACK);
	} else {
		out->set_seq(in->ack()).set_flag(RST);
	}
	tcp.transmit();
	return 1;
}
/////////////////////////////////////////////////////////////////////


/*
	LISTEN
*/

void Connection::Listen::open(Connection& tcp, bool active) {
	if(!tcp.remote().is_empty()) {
		auto out = tcp.outgoing_packet();
		auto& tcb = tcp.tcb();
		tcb.ISS = tcp.generate_iss();
		out->set_seq(tcb.ISS).set_flag(SYN);
		tcb.SND.UNA = tcb.ISS;
		tcb.SND.NXT = tcb.ISS+1;
		tcp.transmit();
		tcp.set_state(SynSent::instance());	
	} else {
		throw TCPException{"No remote host set."};
	}
}

int Connection::Listen::handle(Connection& tcp, TCP::Packet_ptr in) {
	if(in->isset(RST)) {
		// ignore
		return 0;
	}
	auto out = tcp.outgoing_packet();
	if(in->isset(ACK)) {
		out->set_seq(in->ack()).set_flag(RST);
		tcp.transmit();
		return 1;
	}
	if(in->isset(SYN)) {
		auto tcb = tcp.tcb();
		/*
		// Security stuff, don't know yet.
		if(p.PRC > tcb.PRC)
			tcb.PRC = p.PRC;
		*/
		tcb.RCV.NXT 	= in->seq()+1;
		tcb.IRS 		= in->seq();
		tcb.ISS 		= tcp.generate_iss();
		tcb.SND.NXT 	= tcb.ISS+1;
		tcb.SND.UNA 	= tcb.ISS;

		out->set_seq(tcb.ISS).set_ack(tcb.RCV.NXT).set_flags(SYN | ACK);
		tcp.transmit();
		tcp.set_state(SynReceived::instance());
		return 1;
	}
}
/////////////////////////////////////////////////////////////////////


/* 
	SYN-SENT
*/

void Connection::SynSent::send(Connection& tcp, const std::string& data, bool push) {
	tcp.queue_send(data);
}


int Connection::SynSent::handle(Connection& tcp, TCP::Packet_ptr in) {
	auto& tcb = tcp.tcb();
	// 1. check ACK
	if(in->isset(ACK)) {
		//  If SEG.ACK =< ISS, or SEG.ACK > SND.NXT
		if(in->ack() <= tcb.ISS or in->ack() > tcb.SND.NXT) {
			// send a reset 
			if(!in->isset(RST)) {
				auto out = tcp.outgoing_packet();
				out->set_seq(in->ack()).set_flag(RST);
				tcp.transmit();
				return 1;
			}
			// (unless the RST bit is set, if so drop the segment and return)
			else {
				tcp.drop(in);
				return 0;
			}
			// If SND.UNA =< SEG.ACK =< SND.NXT then the ACK is acceptable.
		}
	}
	// 2. check RST
	if(in->isset(RST)) {
		if(in->isset(ACK)) {
      		tcp.signal_error(TCPException{"Connection reset."});
      		tcp.drop(in);
      		//tcp.invoke_close();
      		return 0;
		} else {
			tcp.drop(in);
			return 0;
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
    */
    if(in->isset(SYN)) {
    	tcb.RCV.NXT		= in->seq()+1;
    	tcb.IRS 		= in->seq();
    	tcb.SND.UNA 	= in->ack();
    	// (our SYN has been ACKed)
    	if(tcb.SND.UNA > tcb.ISS) {
    		auto out = tcp.outgoing_packet();
    		out->set_seq(tcb.SND.NXT).set_ack(tcb.RCV.NXT).set_flag(ACK);
    		tcp.transmit();
    		tcp.set_state(Connection::Established::instance());
    		return 1;
    		// return tcp.receive(in); // State is now established.
    		/* 
	    		If there are other controls or
	        	text in the segment then continue processing at the sixth step
	        	below where the URG bit is checked, otherwise return.

	        	If the URG bit is set, RCV.UP <- max(RCV.UP,SEG.UP), and signal
		        the user that the remote side has urgent data if the urgent
		        pointer (RCV.UP) is in advance of the data consumed.  If the
		        user has already been signaled (or is still in the "urgent
		        mode") for this continuous sequence of urgent data, do not
		        signal the user again.
        	*/
        	/*if(!) {
        		return 1;
        	}*/
    	}
    	// Otherwise enter SYN-RECEIVED, form a SYN,ACK segment <SEQ=ISS><ACK=RCV.NXT><CTL=SYN,ACK>
    	else {
    		auto out = tcp.outgoing_packet();
    		out->set_seq(tcb.ISS).set_ack(tcb.RCV.NXT).set_flags(SYN | ACK);
    		tcp.transmit();
    		tcp.set_state(Connection::SynReceived::instance());
    		if(in->has_data()) {
    			queue_receive(in->data());
    		}
    		return 1;
    		/*
				If there are other controls or text in the
        		segment, queue them for processing after the ESTABLISHED state
        		has been reached, return.

        		HOW? return tcp.receive(in); ?
    		*/
    		
    	}
    }
    tcp.drop(in);
    return 0;
}
/////////////////////////////////////////////////////////////////////


/*
	SYN-RECEIVED
*/

void Connection::SynReceived::send(Connection& tcp, const std::string& data, bool push) {
	tcp.queue_send(data);
}

int Connection::SynReceived::handle(Connection& tcp, TCP::Packet_ptr in) {
	// 1. check sequence
	if(!acceptable(tcp, in)) {
		// DRY, extract code from handle to function/acceptable function
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
      	// ..... Hmmm.. need to come up with a good hack for this one. Could be to store previous_state on Connection. set_state() { previous_state = state_; state_ = state; }
	}
	// 3. check security

	// 4. check SYN
	if(in->isset(SYN)) {
		// send RESET
		tcp.signal_disconnect("Connection reset.");
    	//close();
    	return 0;
	}
	auto& tcb = tcp.tcb();
	// 5. check ACK
	if(!in->isset(ACK)) {
		tcp.drop(in);
		return 0;
	}
	// ACK isset
	else {
        /*
        	If SND.UNA =< SEG.ACK =< SND.NXT then enter ESTABLISHED state
          	and continue processing.
      	*/
		if(tcb.SND.UNA <= in->ack() and in->ack() <= tcb.SND.NXT) {
			tcp.set_state(Connection::Established::instance());
			return tcp.handle(in); // TODO: Is this possible, or do i need to call state directly?
		}
		/*
			If the segment acknowledgment is not acceptable, form a
            reset segment, <SEQ=SEG.ACK><CTL=RST> and send it.
        */
		else {
			tcp.outgoing_packet()->set_seq(in->ack())->set_flag(RST);
			tcp.transmit();
		}
	}

	// 8. check FIN
	if(in->isset(FIN)) {
		// DRY - same as ESTABLISHED
		/*
			If the FIN bit is set, signal the user "connection closing" and
      		return any pending RECEIVEs with same message, advance RCV.NXT
      		over the FIN, and send an acknowledgment for the FIN.  Note that
      		FIN implies PUSH for any segment text not yet delivered to the
      		user.
      	*/
  		//TCP::Seq fin = in->seq() + in->data_length();
		tcp.signal_disconnect("Connection closing.");
		// Advance RCV.NXT over the FIN?
		tcb.RCV.NXT++;
		// Send ACK for FIN ???
		auto out = tcp.outgoing_packet();
		tcp.transmit();
		tcp.set_state(Connection::CloseWait::instance());
	}
}
/////////////////////////////////////////////////////////////////////


/*
	ESTABLISHED
*/

int Connection::Established::handle(Connection& tcp, TCP::Packet_ptr in) {
	// 1. Check sequence number

	/*
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
    */
    
    auto& tcb = tcp.tcb();

    if(! acceptable(tcp, in) ) {
    	auto out = tcp.outgoing_packet();
    	out->set_seq(tcb.SND.NXT).set_ack(tcb.RCV.NXT).set_flag(ACK);
    	tcp.transmit();
    	tcp.drop(in);
    	tcp.signal_disconnect("Connection reset.");
    	return 0;
    }

    // 2. check RST
    if( in->isset(RST) ) {
    	// signal Connection reset
    	//close();
    	return 0;
    }

    // 3. check for security

    // 4. check SYN
    if( in->isset(SYN) ) {
    	// send RESET
    	tcp.signal_disconnect("Connection reset.");
    	//close();
    	return 0;
    }

    // 5. ACK bit
    if( !in->isset(ACK) ) {
    	tcp.drop(in);
    	return 0;
    } 
    // ACK isset
    else {
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
        bool check = ( tcb.SND.UNA < in->ack() and in->ack() <= tcb.SND.NXT );
		if( tcb.SND.UNA < in->ack() and in->ack() <= tcb.SND.NXT ) {
			tcb.SND.UNA = in->ack();
			// return that buffer has been SENT - currently no support to receipt sent buffer.

			/*
				If SND.UNA < SEG.ACK =< SND.NXT, the send window should be
          		updated.  If (SND.WL1 < SEG.SEQ or (SND.WL1 = SEG.SEQ and
          		SND.WL2 =< SEG.ACK)), set SND.WND <- SEG.WND, set
          		SND.WL1 <- SEG.SEQ, and set SND.WL2 <- SEG.ACK.
			*/
          	if( tcb.SND.WL1 < in->seq() or ( tcb.SND.WL1 = in->seq() and tcb.SND.WL2 <= in->ack() ) ) {
          		tcb.SND.WND = in->win();
          		tcb.SND.WL1 = in->seq();
          		tcb.SND.WL2 = in->ack();
          	}
          	/*
      			Note that SND.WND is an offset from SND.UNA, that SND.WL1
	          	records the sequence number of the last segment used to update
	          	SND.WND, and that SND.WL2 records the acknowledgment number of
	          	the last segment used to update SND.WND.  The check here
	          	prevents using old segments to update the window.
          	*/

		} else if( in->ack() < tcb.SND.UNA ) {
			// ignore.
		} else if( in->ack() > tcb.SND.NXT ) {
			// Send ACK
			tcp.transmit();
			tcp.drop(in);
			return 0;
		}

		if( tcb.SND.UNA < in->ack() and in->ack() <= tcb.SND.NXT ) {

		}
    }
    // 6. check URG
    if(in->isset(URG)) {
    	/*
	    	If the URG bit is set, RCV.UP <- max(RCV.UP,SEG.UP), and signal
	        the user that the remote side has urgent data if the urgent
	        pointer (RCV.UP) is in advance of the data consumed.  If the
	        user has already been signaled (or is still in the "urgent
	        mode") for this continuous sequence of urgent data, do not
	        signal the user again.
        */
    }

    // 7. proccess the segment text
    /*
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
    if(in->has_data()) {
    	tcp.queue_receive(in->data());
		if(in->isset(PSH)) {
			tcp.signal_data(true);
		}
		auto out = tcp.outgoing_packet();
		out->set_seq(tcb.SND.NXT).set_ack(tcb.RCV.NXT).set_flag(ACK);
		tcp.transmit();	
    }
	

	// 8. Check FIN bit
	/*
	  If the FIN bit is set, signal the user "connection closing" and
      return any pending RECEIVEs with same message, advance RCV.NXT
      over the FIN, and send an acknowledgment for the FIN.  Note that
      FIN implies PUSH for any segment text not yet delivered to the
      user.
	*/
	if(in->isset(FIN)) {
		//TCP::Seq fin = in->seq() + in->data_length();
		tcp.signal_disconnect("Connection closing.");
		// Advance RCV.NXT over the FIN?
		tcb.RCV.NXT++;
		// Send ACK for FIN ???
		auto out = tcp.outgoing_packet();
		tcp.transmit();
		tcp.set_state(Connection::CloseWait::instance());
	}
}
/////////////////////////////////////////////////////////////////////


