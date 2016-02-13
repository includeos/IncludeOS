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
#define DEBUG 1
#define DEBUG2

#include <net/tcp_connection_states.hpp>
#include <algorithm>

using namespace std;


/////////////////////////////////////////////////////////////////////
/*
	COMMON STATE FUNCTIONS
*/
/////////////////////////////////////////////////////////////////////

/*
	1. Check Sequence
*/
/*
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

/*
	TODO: Optimize this one. It checks for the same things.
*/
bool Connection::State::check_seq(Connection& tcp, TCP::Packet_ptr in) {
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
	else if( in->seq() + in->data_length()-1 > tcb.RCV.NXT+tcb.RCV.WND ) {
		acceptable = false;
	}
	// #4
	else if( (tcb.RCV.NXT <= in->seq() and in->seq() < tcb.RCV.NXT + tcb.RCV.WND)
    	or ( tcb.RCV.NXT <= in->seq()+in->data_length()-1 and in->seq()+in->data_length()-1 < tcb.RCV.NXT+tcb.RCV.WND ) ) {
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
			tcp.outgoing_packet()->set_seq(tcb.SND.NXT).set_ack(tcb.RCV.NXT).set_flag(ACK);
			tcp.transmit();
		}
		tcp.drop(in, "Unacceptable SEQ.");
		return false;
	}
	printf("<Connection::State::check_seq> Acceptable SEQ: %u \n", in->seq());
	// is acceptable.
	return true;
}


/*
	4. Check SYN
*/

/*
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
void Connection::State::unallowed_syn_reset_connection(Connection& tcp, TCP::Packet_ptr in) {
	assert(in->isset(SYN));
	// Not sure if this is the correct way to send a "reset response"
	tcp.outgoing_packet()->set_seq(in->ack()).set_flag(RST);
	tcp.transmit();
	tcp.signal_disconnect("Connection reset.");
}


/*
	5. Check ACK
*/
bool Connection::State::check_ack(Connection& tcp, TCP::Packet_ptr in) {
    // 5. ACK bit
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
		if( tcb.SND.UNA < in->ack() and in->ack() <= tcb.SND.NXT ) {
			tcb.SND.UNA = in->ack();
			// tcp.signal_sent();
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

		}
		/* If the ACK acks something not yet sent (SEG.ACK > SND.NXT) then send an ACK, drop the segment, and return. */
		else if( in->ack() > tcb.SND.NXT ) {
			tcp.outgoing_packet()->set_flag(ACK);
			tcp.transmit();
			tcp.drop(in, "ACK > SND.NXT");
			return false;
		}
		/* If the ACK is a duplicate (SEG.ACK < SND.UNA), it can be ignored. */
		/*else if( in->ack() < tcb.SND.UNA ) {
			// ignore.
		}*/
		return true;
    }
    // ACK not set.
    else {
    	tcp.drop(in, "!ACK"});
    	return false;
    }
}


/*
	7. Process the segment text
*/
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
void Connection::State::process_segment(Connection& conn, TCP::Packet_ptr in) {
    
    // TODO: Piggyback ACK & data.
    
    assert(in->has_data());
	auto& tcb = tcp.tcb();
	printf("<TCP::Connection::State::handle> Received packet with DATA-LENGTH: %i. Copy to receive buffer. \n", in->data_length());
	tcp.queue_receive(in->data());
	if(in->isset(PSH)) {
		tcp.signal_data(true);
	}
	/*
		Once the TCP takes responsibility for the data it advances
        RCV.NXT over the data accepted, and adjusts RCV.WND as
        apporopriate to the current buffer availability.  The total of
        RCV.NXT and RCV.WND should not be reduced.
    */
	tcb.RCV.NXT += in->data_length();
	printf("<TCP::Connection::State::handle> Advanced RCV.NXT: %u. SND.NXT = %u \n", tcb.RCV.NXT, tcb.SND.NXT);
	// TODO: Adjust RCV.WND, section 3.7
	//tcp.push_data(true); // TODO: can't send from everywhere.
	tcp.outgoing_packet()->set_seq(tcb.SND.NXT).set_ack(tcb.RCV.NXT).set_flag(ACK);
	tcp.transmit();	
}

/*
	8. Check FIN
*/

/*
	If the FIN bit is set, signal the user "connection closing" and
	return any pending RECEIVEs with same message, advance RCV.NXT
	over the FIN, and send an acknowledgment for the FIN.  Note that
	FIN implies PUSH for any segment text not yet delivered to the
	user.
*/

void Connection::State::process_fin(Connection& tcp, TCP::Packet_ptr in) {
    assert(in->isset(FIN));
    auto& tcb = tcp.tcb();
	tcp.signal_disconnect("Connection closing.");
	// Advance RCV.NXT over the FIN?
	//tcb.RCV.NXT++;
	auto fin = in->seq() + in->data_length();
	tcb.RCV.NXT += fin;
	tcp.outgoing_packet()->set_ack(tcb.RCV.NXT)->set_flag(ACK);
	tcp.transmit();
	// tcp.signal_data(true);
}

/*
	Fallback.
*/

void Connection::State::open(Connection& tcp, bool active) {
	throw TCPException{"Connection already exists."};
}

void Connection::State::send(Connection& tcp, const std::string& data, bool push) {
	throw TCPException{"Connection is not open."};
}

std::string Connection::State::receive(Connection& tcp, size_t buffer_size) {
	return "";
}

void Connection::State::close(Connection& tcp) {
	// Dirty close
}

int Connection::State::handle(Connection& tcp, TCP::Packet_ptr in) {
	tcp.drop(in, "STATELESS");
}

string Connection::State::to_string() const {
	return "STATELESS";
}
/////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////
/*
	CLOSED
*/
/////////////////////////////////////////////////////////////////////

void Connection::Closed::open(Connection& tcp, bool active) {
	if(active) {
		// There is a remote host
		if(!tcp.remote().is_empty()) {
			auto& tcb = tcp.tcb();
			tcb.ISS = tcp.generate_iss();
			tcp.outgoing_packet()->set_seq(tcb.ISS).set_flag(SYN);
			tcb.SND.UNA = tcb.ISS;
			tcb.SND.NXT = tcb.ISS+1;
			tcp.transmit();
			tcp.set_state(SynSent::instance());	
		} else {
			throw TCPException{"No remote host set."};
		}
	} else {
		tcp.set_state(Connection::Listen::instance());
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


/////////////////////////////////////////////////////////////////////
/*
	LISTEN
*/
/////////////////////////////////////////////////////////////////////

void Connection::Listen::open(Connection& tcp, bool active) {
	if(!tcp.remote().is_empty()) {
		auto& tcb = tcp.tcb();
		tcb.ISS = tcp.generate_iss();
		tcp.outgoing_packet()->set_seq(tcb.ISS).set_flag(SYN);
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
	if(in->isset(ACK)) {
		tcp.outgoing_packet()->set_seq(in->ack()).set_flag(RST);
		tcp.transmit();
		return 1;
	}
	if(in->isset(SYN)) {
		auto& tcb = tcp.tcb();
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
		printf("<TCP::Connection::Listen::handle> Received SYN Packet: %s TCB Updated:\n %s \n", in->to_string().c_str(), tcp.tcb().to_string().c_str());

		tcp.outgoing_packet()->set_seq(tcb.ISS).set_ack(tcb.RCV.NXT).set_flags(SYN | ACK);
		tcp.transmit();
		tcp.set_state(SynReceived::instance());
		return 1;
	}
	return 0;
}
/////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////
/* 
	SYN-SENT
*/
/////////////////////////////////////////////////////////////////////

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
				tcp.drop(in, "RST");
				return 0;
			}
			// If SND.UNA =< SEG.ACK =< SND.NXT then the ACK is acceptable.
		}
	}
	// 2. check RST
	if(in->isset(RST)) {
		if(in->isset(ACK)) {
      		tcp.signal_error(TCPException{"Connection reset."});
      		tcp.drop(in, "RST with acceptable ACK");
      		return -1;
		} else {
			tcp.drop(in, "RST");
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
    if(in->isset(SYN)) {
    	tcb.RCV.NXT		= in->seq()+1;
    	tcb.IRS 		= in->seq();
    	tcb.SND.UNA 	= in->ack();
    	
    	// (our SYN has been ACKed)
    	if(tcb.SND.UNA > tcb.ISS) {
    		tcp.outgoing_packet()->set_seq(tcb.SND.NXT).set_ack(tcb.RCV.NXT).set_flag(ACK);
    		tcp.transmit();
    		tcp.set_state(Connection::Established::instance());
    		//return tcp.state().handle(tcp, in);
    		return 1;
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
    		tcp.outgoing_packet()->set_seq(tcb.ISS).set_ack(tcb.RCV.NXT).set_flags(SYN | ACK);
    		tcp.transmit();
    		tcp.set_state(Connection::SynReceived::instance());
    		if(in->has_data()) {
    			tcp.queue_receive(in->data());
    			// Advance RCV.NXT ??
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


/////////////////////////////////////////////////////////////////////
/*
	SYN-RECEIVED
*/
/////////////////////////////////////////////////////////////////////

void Connection::SynReceived::send(Connection& tcp, const std::string& data, bool push) {
	tcp.queue_send(data);
}

int Connection::SynReceived::handle(Connection& tcp, TCP::Packet_ptr in) {
	// 1. check sequence
	if(! check_seq(tcp, in) ) {
		return 0;
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
      	/*if(tcp.prev_state() == Connection::Listen::instance()) {
        	// Since we create a new connection when it starts listening, we don't wanna do this, but just delete it.
        	//tcp.set_state(Connection::Listen::instance());
        }*/
      	if(tcp.prev_state().to_string() == Connection::SynSent::instance().to_string()) {
      		tcp.signal_disconnect("Connection refused.");
      	}

      	// close();
      	return -1;
        
        
	}
	// 3. check security

	// 4. check SYN
	if( in->isset(SYN) ) {
		unallowed_syn_reset_connection(tcp, in);
		return -1;
	}

	auto& tcb = tcp.tcb();
	// 5. check ACK
	if(in->isset(ACK)) {	
		/*
        	If SND.UNA =< SEG.ACK =< SND.NXT then enter ESTABLISHED state
          	and continue processing.
      	*/
		if(tcb.SND.UNA <= in->ack() and in->ack() <= tcb.SND.NXT) {
			printf("<TCP::Connection::SynReceived::handle> SND.UNA =< SEG.ACK =< SND.NXT, continue in ESTABLISHED. \n");
			tcp.set_state(Connection::Established::instance());
			return tcp.state().handle(tcp,in); // TODO: Fix. This is kinda bad, need to make the above steps again.
		}
		/*
			If the segment acknowledgment is not acceptable, form a
            reset segment, <SEQ=SEG.ACK><CTL=RST> and send it.
        */
		else {
			tcp.outgoing_packet()->set_seq(in->ack()).set_flag(RST);
			tcp.transmit();
		}
	}
	// ACK is missing
	else {
        tcp.drop(in, "SYN-RCV: !ACK");
		return 0;
	}

	// 8. check FIN
	if(in->isset(FIN)) {
		process_fin(tcp, in);
		tcp.set_state(Connection::CloseWait::instance());
		return 0;
	}
	return 0;
}
/////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////
/*
	ESTABLISHED
*/
/////////////////////////////////////////////////////////////////////

/*
	Segmentize the buffer and send it with a piggybacked
  	acknowledgment (acknowledgment value = RCV.NXT).  If there is
  	insufficient space to remember this buffer, simply return "error:
  	insufficient resources".

  	If the urgent flag is set, then SND.UP <- SND.NXT-1 and set the
  	urgent pointer in the outgoing segments.	
*/
void Connection::Established::send(Connection& tcp, const std::string& data, bool push) {
	printf("<TCP::Connection::Established::send> Sending data with the length of %u. PUSH: %d \n", data.size(), push);
    /*
    	3.7 Data Communication
		The sender of data keeps track of the next sequence number to use in
		the variable SND.NXT.  The receiver of data keeps track of the next
		sequence number to expect in the variable RCV.NXT.  The sender of data
		keeps track of the oldest unacknowledged sequence number in the
		variable SND.UNA.  If the data flow is momentarily idle and all data
		sent has been acknowledged then the three variables will be equal.

		When the sender creates a segment and transmits it the sender advances
		SND.NXT.  When the receiver accepts a segment it advances RCV.NXT and
		sends an acknowledgment.  When the data sender receives an
		acknowledgment it advances SND.UNA.  The extent to which the values of
		these variables differ is a measure of the delay in the communication.
		The amount by which the variables are advanced is the length of the
		data in the segment.  Note that once in the ESTABLISHED state all
		segments must carry current acknowledgment information.
	*/
	tcp.queue_send(data);
  	unsigned int remaining{tcp.send_buffer().size()};
  	auto& tcb = tcp.tcb();
	// TODO: ACK and TCB stuff.
	do {
		auto packet = tcp.outgoing_packet();
		
		remaining -= packet->set_seq(tcb.SND.NXT).set_ack(tcb.RCV.NXT).set_flag(ACK).fill(tcp.send_buffer_);
		
		// If last packet and PUSH
		if(!remaining and push)
			packet->set_flag(PSH);
		tcp.transmit();
		// Advance SND.NXT
		tcb.SND.NXT += packet->data_length();

	} while(remaining);
	
}

std::string Connection::Established::receive(Connection& tcp, size_t buffer_size) {
	auto& buffer = tcp.receive_buffer_;

	return {buffer.begin(), buffer.end()};
}

void Connection::Established::close(Connection& tcp) {
	auto& tcb = tcp.tcb();
	tcp.outgoing_packet()->set_seq(tcb.SND.NXT).set_ack(tcb.RCV.NXT).set_flags(ACK | FIN);
	tcp.transmit();
	tcp.set_state(Connection::FinWait1::instance());
}

int Connection::Established::handle(Connection& tcp, TCP::Packet_ptr in) {
	// 1. check SEQ
    if(! check_seq(tcp, in) ) {
    	return 0;
    }

    // 2. check RST
    if( in->isset(RST) ) {
    	tcp.signal_disconnect("Connection reset.");
    	return -1; // close
    }

    // 3. check security

    // 4. check SYN
	if( in->isset(SYN) ) {
		unallowed_syn_reset_connection(tcp, in);
		return -1;
	}

    // 5. check ACK
    if( ! check_ack(tcp, in) ) {
    	return 0;
    }
    // 6. check URG - DEPRECATED

    // 7. proccess the segment text
    if(in->has_data()) {
    	process_segment(tcp, in);	
    }

	// 8. check FIN bit
	if(in->isset(FIN)) {
		process_fin(tcp, in);
		tcp.set_state(Connection::CloseWait::instance());
		return 0;
	}
	return 0;
}
/////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////
/*
	FIN-WAIT-1
*/
/////////////////////////////////////////////////////////////////////

int Connection::FinWait1::handle(Connection& tcp, TCP::Packet_ptr in) {
	// 1. Check sequence number
    if(! check_seq(tcp, in) ) {
    	return 0;
    }

    // 2. check RST
    if( in->isset(RST) ) {
    	tcp.signal_disconnect("Connection reset.");
    	return -1; // close
    }

    // 4. check SYN
	if( in->isset(SYN) ) {
		unallowed_syn_reset_connection(tcp, in);
		return -1;
	}

    // 5. check ACK
    if( ! check_ack(tcp, in) ) {
    	return 0;
    }

    // 7. proccess the segment text
    if(in->has_data()) {
    	process_segment(tcp, in);	
    }

    // 8. check FIN
    if(in->isset(FIN)) {
    	process_fin(tcp, in);
    	/*
			If our FIN has been ACKed (perhaps in this segment), then
          	enter TIME-WAIT, start the time-wait timer, turn off the other
          	timers; otherwise enter the CLOSING state.
    	*/
      	// TODO: Implement above.
    	return 0;
    }
}
/////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////
/*
	FIN-WAIT-2
*/
/////////////////////////////////////////////////////////////////////

int Connection::FinWait2::handle(Connection& tcp, TCP::Packet_ptr in) {
	// 1. check SEQ
    if(! check_seq(tcp, in) ) {
    	return 0;
    }

    // 2. check RST
    if( in->isset(RST) ) {
    	tcp.signal_disconnect("Connection reset.");
    	return -1; // close
    }

    // 4. check SYN
	if( in->isset(SYN) ) {
		unallowed_syn_reset_connection(tcp, in);
		return -1;
	}

    // 5. check ACK
    if( ! check_ack(tcp, in) ) {
    	return 0;
    }

    // 7. proccess the segment text
    if(in->has_data()) {
    	process_segment(tcp, in);
    }

    // 8. check FIN
    if(in->isset(FIN)) {
    	process_fin(tcp, in);
    	/*
			Enter the TIME-WAIT state.  Start the time-wait timer, turn off the other timers.
    	*/
      	// TODO: Implement above.
    	return 0;
    }
}
/////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////
/*
	CLOSE-WAIT
*/
/////////////////////////////////////////////////////////////////////

int Connection::CloseWait::handle(Connection& tcp, TCP::Packet_ptr in) {
	// 1. check SEQ
    if(! check_seq(tcp, in) ) {
    	return 0;
    }

    // 2. check RST
    if( in->isset(RST) ) {
    	tcp.signal_disconnect("Connection reset.");
    	return -1; // close
    }

    // 4. check SYN
	if( in->isset(SYN) ) {
		unallowed_syn_reset_connection(tcp, in);
		return -1;
	}

    // 5. check ACK
    if( ! check_ack(tcp, in) ) {
    	return 0;
    }

    // 7. proccess the segment text
    // This should not occur, since a FIN has been received from the remote side.  Ignore the segment text.

    // 8. check FIN
    if(in->isset(FIN)) {
    	process_fin(tcp, in);
    	// Remain in state
    	return 0;
    }
}
/////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////
/*
	CLOSING
*/
/////////////////////////////////////////////////////////////////////

int Connection::Closing::handle(Connection& tcp, TCP::Packet_ptr in) {
	// 1. Check sequence number
    if(! check_seq(tcp, in) ) {
    	return 0;
    }

    // 2. check RST
    if( in->isset(RST) ) {
    	return -1; // close
    }

    // 4. check SYN
    if( in->isset(SYN) ) {
		unallowed_syn_reset_connection(tcp, in);
		return -1;
	}

    // 7. proccess the segment text
    // This should not occur, since a FIN has been received from the remote side.  Ignore the segment text.

    // 8. check FIN
    if(in->isset(FIN)) {
    	process_fin(tcp, in);
    	// Remain in state
    	return 0;
    }
}
/////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////
/*
	LAST-ACK
*/
/////////////////////////////////////////////////////////////////////

int Connection::LastAck::handle(Connection& tcp, TCP::Packet_ptr in) {
	// 1. Check sequence number
    if(! check_seq(tcp, in) ) {
    	return 0;
    }

    // 2. check RST
    if( in->isset(RST) ) {
    	return -1; // close
    }

    // 4. check SYN
    if( in->isset(SYN) ) {
		unallowed_syn_reset_connection(tcp, in);
		return -1;
	}

    // 7. proccess the segment text
    // This should not occur, since a FIN has been received from the remote side.  Ignore the segment text.

    // 8. check FIN
    if(in->isset(FIN)) {
    	process_fin(tcp, in);
    	// Remain in state
    	return 0;
    }
}
/////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////
/*
	TIME-WAIT
*/
/////////////////////////////////////////////////////////////////////

int Connection::TimeWait::handle(Connection& tcp, TCP::Packet_ptr in) {
	// 1. Check sequence number
    if(! check_seq(tcp, in) ) {
    	return 0;
    }

    // 2. check RST
    if( in->isset(RST) ) {
    	return -1; // close
    }

    // 4. check SYN
    if( in->isset(SYN) ) {
		unallowed_syn_reset_connection(tcp, in);
		return -1;
	}

    // 7. proccess the segment text
    // This should not occur, since a FIN has been received from the remote side.  Ignore the segment text.

    // 8. check FIN
    if(in->isset(FIN)) {
    	process_fin(tcp, in);
    	// Remain in state
    	// TODO: Restart the 2 MSL time-wait timeout.
    	return 0;
    }
    
}
/////////////////////////////////////////////////////////////////////
