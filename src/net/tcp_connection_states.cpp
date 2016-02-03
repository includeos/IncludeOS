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

void Connection::State::open(Connection& tcp, bool active) {
	// Connection already exists
}

void Connection::State::close(Connection& tcp) {
	// Dirty close
}

int Connection::State::handle(Connection& tcp, TCP::Packet_ptr in) {
	// Drop packet
	tcp.drop(in);
}

string Connection::State::to_string() const {
	return "STATELESS";
}

// CLOSED

void Connection::Closed::open(Connection& tcp, bool active) {
	if(active) {
		// There is a remote host
		if(!tcp.remote().is_empty()) {
			auto out = tcp.create_outgoing_packet();
			auto& tcb = tcp.tcb();
			out->set_seq(tcb.ISS).set_flag(SYN);
			tcb.SND.UNA = tcb.ISS;
			tcb.SND.NXT = tcb.ISS+1;
			tcp.transmit(out);
			tcp.set_state(SynSent::instance());	
		} else {
			// throw error
			return; // TODO: Remove silent return
		}
	} else {
		tcp.set_state(Listen::instance());
	}
}

int Connection::Closed::handle(Connection& tcp, TCP::Packet_ptr in) {
	if(in->isset(RST)) {
		return 0;
	}
	auto out = tcp.create_outgoing_packet();
	if(!in->isset(ACK)) {
		out->set_seq(0).set_ack(in->seq() + in->data_length()).set_flags(RST | ACK);
	} else {
		out->set_seq(in->ack()).set_flag(RST);
	}
	tcp.transmit(out);
	return 1;
}

// LISTEN

void Connection::Listen::open(Connection& tcp, bool active) {
	if(!tcp.remote().is_empty()) {
		auto out = tcp.create_outgoing_packet();
		auto& tcb = tcp.tcb();
		tcb.ISS = tcp.generate_iss();
		out->set_seq(tcb.ISS).set_flag(SYN);
		tcb.SND.UNA = tcb.ISS;
		tcb.SND.NXT = tcb.ISS+1;
		tcp.transmit(out);
		tcp.set_state(SynSent::instance());	
	} else {
		// throw error
		return; // TODO: Remove silent return
	}
}

int Connection::Listen::handle(Connection& tcp, TCP::Packet_ptr in) {
	if(in->isset(RST)) {
		// ignore
		return 0;
	}
	auto out = tcp.create_outgoing_packet();
	if(in->isset(ACK)) {
		out->set_seq(in->ack()).set_flag(RST);
		return 1;
	}
	if(in->isset(SYN)) {
		auto tcb = tcp.tcb();
		/*
		// Security stuff, don't know yet.
		if(p.PRC > tcb.PRC)
			tcb.PRC = p.PRC;
		*/
		tcb.RCV.NXT = in->seq()+1;
		tcb.IRS = in->seq();
		tcb.ISS = tcp.generate_iss();
		tcb.SND.NXT = tcb.ISS+1;
		tcb.SND.UNA = tcb.ISS;
		out->set_seq(tcb.ISS).set_ack(tcb.RCV.NXT).set_flags(SYN | ACK);
		// SEND SEGMENT/PACKET
		tcp.transmit(out);
		tcp.set_state(SynReceived::instance());
		return 1;
	}
}

// SYN-SENT

int Connection::SynSent::handle(Connection& tcp, TCP::Packet_ptr in) {
	auto tcb = tcp.tcb();
	if(in->isset(ACK)) {
		if(in->ack() <= tcb.ISS or in->ack() > tcb.SND.NXT) {
			if(in->isset(RST)) {
				//drop();
			} else {
				// send reset
			}
		} else if (tcb.SND.UNA <= in->ack() and in->ack() <= tcb.SND.NXT) {
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