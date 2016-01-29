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
Connection::Connection(TCP& parent, Socket& local, Socket& remote, TCP::Seq iss) :
	parent_(parent),
	local_(local), 
	remote_(remote_),
	state_(Connection::Closed::instance()),
	control_block()
{
	control_block_.ISS = iss;

	// Always open a new Connection????
	//state_->open(*this);
}

/*
	This is most likely used in a PASSIVE open
*/
Connection::Connection(TCP& parent, Socket& local, TCP::Seq iss) :
	parent_(parent),
	local_(local), 
	remote_(TCP::Socket()),
	state_(Connection::Closed::instance()),
	control_block()
{
	control_block.ISS = iss;
	// I can also say: parent.generate_iss(), and drop iss from constructor.
}

TCP::Packet_ptr Connection::createPacket() {
	auto pckt_ptr = parent_.inet_.createPacket(sizeof(TCP::Full_header));
	auto packet = std::static_pointer_cast<TCP::Packet>(pckt_ptr);
	
	packet->init();
	// Set Source (local == the current connection)
	packet->set_src_port(local_.port())->set_src(local_.address());
	// Set Destination (remote)
	packet->set_dst_port(remote_.port())->set_dst(remote_.address());
	// Clear flags (Is this needed...?)
	packet->header().clear_flags();

	return packet;
}

void Connection::receive(TCP::Packet_ptr incoming) {
	// Let state handle what to do when incoming packet arrives, and modify the outgoing packet.
	if(state_.handle(*this, incoming, createPacket())) {
		/*if(is_connected()) {
			on_accept_handler(*this);
		}*/
		transmit(outgoing);
	} else {
		//drop(packet);
		//error handler
	}
}

void Connection::open(bool active = false) {
	// TODO: Add support for OPEN/PASSIVE
	state_.open(active);
}

Connection::TCB& Connection::tcb() const {
	return control_block;
}

Connection::Tuple& Connection::tuple() {
	Tuple tuple = {
		local = local_;
		remote = remote_;
	}
	return tuple;
}

void Connection::transmit(TCP::Packet out) {
	parent.transmit(out);
}

Connection::~Connection() {
	// Do all necessary clean up.
	// Free up buffers etc.
}
