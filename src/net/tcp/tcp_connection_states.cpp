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

using namespace std;

// STATE - Fallback if not implemented in state.

void State::open(Connection& tcp) {
	// Cannot open
}

void State::close(Connection& tcp) {
	// Dirty close
}

void State::handle(Connection& tcp, Packet& p) {
	// Drop packet
	//p.die();
	//p.drop();
	close(tcp);
}

string State::to_string() const {
	return "STATELESS";
}

// CLOSED

int Closed::handle(Connection& tcp, TCP::Packet_ptr in, TCP::Packet_ptr out) {
	if(in->isset(RST)) {
		return 0;
	}
	if(!in->isset(ACK)) {
		out->set_seq(0)->set_ack(in->seq() + in->data_length())->set_flags(RST | ACK);
	} else {
		out->set_seq(in->ack())->set_flag(RST);
	}
	return 1;
}

// LISTEN

int Listen::handle(Connection& tcp, TCP::Packet_ptr in, TCP::Packet_ptr out) {
	auto tcb = tcp.tcb();
	if(in->isset(RST)) {
		// ignore
		return 0;
	}
	if(in->isset(ACK)) {
		out->set_seq(in->ack())->set_flag(RST);
		return 1;
	}
	if(p.isset(SYN)) {		
		/*
		// Security stuff, don't know yet.
		if(p.PRC > tcb.PRC)
			tcb.PRC = p.PRC;
		*/
		tcb.RCV_NEXT = p->seq()+1;
		tcb.IRS = p->seq();
		// select ISS; already there.
		tcb.SND_NXT = tcb.ISS+1;
		tcb.SND_UNA = tcb.ISS;
		p->set_seq(tcb.ISS)->set_ack(tcb.RCV_NXT)->set_flags(SYN | ACK);
		// SEND SEGMENT/PACKET
		set_state(tcp, SynReceived::instance());
		return 1;
	}
}

// SYN-SENT

void SynSent::handle(Connection& tcp, TCP::Packet_ptr in, TCP::Packet_ptr out) {
	auto tcb = tcp.tcb();
	if(in->isset(ACK)) {
		if(in->ack() =< tcb.ISS or in->ack() > tcb.SND_NXT) {
			if(in->isset(RST)) {
				//drop();
			} else {
				// send reset
			}
		} else if (tcb.SND_UNA =< in->ack() =< tcb.SND_NXT) {
			// Acceptable ACK, continue.
		}
	}
	if(in->isset(RST)) {
		/*
			If the ACK was acceptable then signal the user "error:
          	connection reset", drop the segment, enter CLOSED state,
          	delete TCB, and return.  Otherwise (no ACK) drop the segment
          	and return.
		*/
	}
}