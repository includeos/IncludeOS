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

using namespace net;
using Connection = TCP::Connection;

using namespace std;

/*
	This is most likely used in a ACTIVE open
*/
Connection::Connection(TCP& host, Socket& local, Socket&& remote, TCP::Seq iss) :
	host_(host),
	local_(local), 
	remote_(remote_),
	state_(Connection::Closed::instance()),
	control_block()
{
	control_block.ISS = iss;

	// Always open a new Connection????
	//state_->open(*this);
}

/*
	This is most likely used in a PASSIVE open
*/
Connection::Connection(TCP& host, Socket& local, TCP::Seq iss) :
	host_(host),
	local_(local), 
	remote_(TCP::Socket()),
	state_(Connection::Closed::instance()),
	control_block()
{
	control_block.ISS = iss;
	// I can also say: host.generate_iss(), and drop iss from constructor.
}

string Connection::read(uint16_t buffer_size) {
	return "implement me";
}

void Connection::write(std::string data) {
	// Implement
}

/*
	If ACTIVE: 
	Need a remote Socket and also a packet to be sent out.
*/
void Connection::open(bool active) {
	// TODO: Add support for OPEN/PASSIVE
	if(active) {
		// No remote host - can't make a ACTIVE open
		if(remote_.is_empty()) {
			// throw exception - missing remote host.
		}
		// Remote host - make an ACTIVE open
		else {
			auto outgoing = create_outgoing_packet();
			state_.open(*this, outgoing); // throw exception from State::open()?
			transmit(outgoing);	
		}
	}
	else {
		state_.open(*this);
	}
}

string Connection::to_string() const {
	ostringstream os;
	os << local_.to_string() << "\t" << remote_.to_string() << "\t" << state_.to_string();
	return os.str();
}

Connection::Tuple Connection::tuple() {
	return {local_, remote_};
}

/*
	Where the magic happens.
*/
void Connection::receive(TCP::Packet_ptr incoming) {
	// Let state handle what to do when incoming packet arrives, and modify the outgoing packet.
	//auto outgoing = create_outgoing_packet(incoming.source());
	auto outgoing = create_outgoing_packet();
	if(state_.handle(*this, incoming, outgoing)) {
		/*if(is_connected()) {
			on_sucess_handler(*this);
		}*/
		transmit(outgoing);
	} else {
		//drop(packet);
		//error handler
	}
}

Connection::~Connection() {
	// Do all necessary clean up.
	// Free up buffers etc.
}


TCP::Packet_ptr Connection::create_outgoing_packet() {
	auto pckt_ptr = (host_.inet_).createPacket(TCP::Packet::HEADERS_SIZE);
	auto packet = TCP::net2tcp(pckt_ptr);
	
	packet->init();
	// Set Source (local == the current connection)
	//packet->set_src_port(local_.port())->set_src(local_.address());
	packet->set_source(local_);
	// Set Destination (remote)
	//packet->set_dst_port(remote_.port())->set_dst(remote_.address());
	packet->set_destination(remote_);
	// Clear flags (Is this needed...?)
	packet->header().clear_flags();

	return packet;
}

void Connection::transmit(TCP::Packet_ptr out) {
	host_.transmit(out);
}


