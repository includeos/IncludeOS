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
Connection::Connection(TCP& host, Socket& local, Socket&& remote) :
	host_(host),
	local_(local), 
	remote_(remote),
	state_(Connection::Closed::instance()),
	control_block(),
	outgoing_packet_(nullptr)
{

}

/*
	This is most likely used in a PASSIVE open
*/
Connection::Connection(TCP& host, Socket& local) :
	host_(host),
	local_(local), 
	remote_(TCP::Socket()),
	state_(Connection::Closed::instance()),
	control_block(),
	outgoing_packet_(nullptr)
{
	
}

string Connection::read(uint16_t buffer_size) {
	//state_.receive()
	return "implement me";
}

void Connection::write(std::string data) {
	state_.send(*this, data);
	/*int remaining{data.size()};

	do {
		remaining -= outgoing_packet().fill(data);
		transmit();
	} while(remaining);*/
	/*do {
		remaining -= last_packet.fill(data);

		if(last_packet.full()) {
			transmit();
		}

		if(remaining) {
			auto packet = outgoing_packet();
			last_packet.chain(packet);
			last_packet = packet;
		}
	} while(remaining);*/
}

int Connection::queue_send(const std::string& data) {
	std::copy(data.begin(), data.end(), back_inserter(send_buffer_));
	return data.size();
}

int Connection::queue_receive(const std::string& data) {
	std::copy(data.begin(), data.end(), back_inserter(receive_buffer_));
	return data.size();
}

void Connection::push_data(bool push) {
	unsigned int remaining{send_buffer_.size()};
	// TODO: ACK and TCB stuff.
	do {
		auto packet = outgoing_packet();
		remaining -= packet->fill(send_buffer_);
		if(!remaining and push)
			packet->set_flag(PSH);
		transmit();
	} while(remaining);
}

/*
	If ACTIVE: 
	Need a remote Socket and also a packet to be sent out.
*/
void Connection::open(bool active) {
	// TODO: Add support for OPEN/PASSIVE
	try {
		state_.open(*this, active);
	}
	// No remote host, or state isnt valid for opening.
	catch (TCPException e) {
		signal_error(e);
	}	
}

void Connection::close() {

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
	if(state_.handle(*this, incoming)) {
		bytes_received_ += incoming->data_length();
		/*if(is_connected()) {
			on_sucess_handler(*this);
		}*/
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
	auto packet = std::static_pointer_cast<TCP::Packet>((host_.inet_).createPacket(TCP::Packet::HEADERS_SIZE));
	
	packet->init();
	// Set Source (local == the current connection)
	packet->set_source(local_);
	// Set Destination (remote)
	packet->set_destination(remote_);
	// Clear flags (Is this needed...?)
	packet->header().clear_flags();

	return packet;
}

void Connection::transmit() {
	host_.transmit(outgoing_packet_);
	bytes_transmitted_ += outgoing_packet_->data_length();
	outgoing_packet_ = nullptr;
}

TCP::Packet_ptr Connection::outgoing_packet() {
	if(outgoing_packet_ == nullptr)
		outgoing_packet_ = create_outgoing_packet();
	return outgoing_packet_;
}

TCP::Seq Connection::generate_iss() {
	return host_.generate_iss();
}
