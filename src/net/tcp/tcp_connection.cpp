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

#include <net/tcp.hpp>
#include <net/tcp_connection_states.hpp>

using Connection = TCP::Connection;

/*
	This is most likely used in a ACTIVE open
*/
Connection::Connection(IPStack& local_stack, Socket& local, Socket& remote, TCP::Seq iss) :
	local_stack_(local_stack),
	local_(local), 
	remote_(remote_),
	state_(Connection::Closed::instance()),
	control_block()
{
	control_block_.ISS = iss;

}

/*
	This is most likely used in a PASSIVE open
*/
Connection::Connection(IPStack& local_stack, Socket& local, TCP::Seq iss) :
	local_stack_(local_stack),
	local_(local), 
	remote_(TCP::Socket()),
	state_(Connection::Closed::instance()),
	control_block()
{
	control_block.ISS = iss;
}



void Connection::receive(TCP::Packet& packet) {
	// The TCP Packet has arrived to destination connection.
	incoming_ = packet;

	if(state_.handle(*this, incoming_)) {
		/*if(is_connected()) {
			on_accept_handler(*this);
		}*/
		transmit(outgoing_);
	} else {
		//drop(packet);
	}
}

Connection::TCB& Connection::tcb() const {
	return control_block;
}

void Connection::transmit(Packet& out) {
	local_stack_.tcp().transmit(out);
}

Connection::~Connection() {
	// Do all necessary clean up.
	// Free up buffers etc.
}
