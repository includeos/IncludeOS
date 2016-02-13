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

#include <net/tcp.hpp>
#include <net/tcp_connection_states.hpp>

using namespace net;
using Connection = TCP::Connection;

using namespace std;

/*
	This is most likely used in a ACTIVE open
*/
Connection::Connection(TCP& host, Socket& local, Socket remote) :
	host_(host),
	local_port_(local.port()), 
	remote_(remote),
	state_(&Connection::Closed::instance()),
	prev_state_(state_),
	control_block(),
	outgoing_packet_(nullptr)
{

}

/*
	This is most likely used in a PASSIVE open
*/
Connection::Connection(TCP& host, Socket& local) :
	host_(host),
	local_port_(local.port()), 
	remote_(TCP::Socket()),
	state_(&Connection::Closed::instance()),
	prev_state_(state_),
	control_block(),
	outgoing_packet_(nullptr)
{
	
}

string Connection::read(size_t buffer_size) {
	return state_->receive(*this, buffer_size);
}

void Connection::write(const std::string& data) {
	printf("<TCP::Connection::write> Writing data with length %i \n", data.size());
	state_->send(*this, data);
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
	/*
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
	do {
		auto packet = outgoing_packet();
		remaining -= packet->set_seq(control_block.SND.NXT).set_ack(control_block.RCV.NXT).set_flag(ACK).fill(send_buffer_);
		if(!remaining and push)
			packet->set_flag(PSH);
		transmit();
	} while(remaining);
}

/*
	If ACTIVE: 
	Need a remote Socket.
*/
void Connection::open(bool active) {
	try {
		printf("<TCP::Connection::open> Trying to open Connection...\n");
		state_->open(*this, active);
	}
	// No remote host, or state isnt valid for opening.
	catch (TCPException e) {
		printf("<TCP::Connection::open> Cannot open Connection. \n");
		signal_error(e);
	}	
}

void Connection::close() {

}

string Connection::to_string() const {
	ostringstream os;
	os << local().to_string() << "\t" << remote_.to_string() << "\t" << state_->to_string();
	return os.str();
}

/*
	Where the magic happens.
*/
void Connection::receive(TCP::Packet_ptr incoming) {
	// Let state handle what to do when incoming packet arrives, and modify the outgoing packet.
	printf("<TCP::Connection::receive> Received incoming TCP Packet on %p: %s \n", 
			this, incoming->to_string().c_str());
	// Change window accordingly. 
	control_block.SND.WND = incoming->win();
	int sig = state_->handle(*this, incoming);
	
	printf("<TCP::Connection::receive> State handle finished with value: %i. If -1, Connection is supposed to close. \n", sig);
}

Connection::~Connection() {
	// Do all necessary clean up.
	// Free up buffers etc.
}


TCP::Packet_ptr Connection::create_outgoing_packet() {
	auto packet = std::static_pointer_cast<TCP::Packet>((host_.inet_).createPacket(TCP::Packet::HEADERS_SIZE));
	
	packet->init();
	// Set Source (local == the current connection)
	packet->set_source(local());
	// Set Destination (remote)
	packet->set_destination(remote_);
	// Clear flags (Is this needed...?)
	//packet->header().clear_flags();
	//packet->set_size(sizeof(TCP::Full_header));
	packet->set_win_size(control_block.SND.WND);
	
	packet->header().set_offset(5); // Hardcoded for now, until support for option.
	// Set SEQ and ACK - I think this is OK..
	packet->set_seq(control_block.SND.NXT).set_ack(control_block.RCV.NXT);
	debug("<TCP::Connection::create_outgoing_packet> Outgoing packet created: %s \n", packet->to_string().c_str());

	return packet;
}

void Connection::transmit() {
	assert(outgoing_packet_ != nullptr);
	assert(! outgoing_packet_->destination().is_empty());

	printf("<TCP::Connection::transmit> Transmitting packet: %s \n", outgoing_packet_->to_string().c_str());
	host_.transmit(outgoing_packet_);
	// Packet is gone. (retransmit timer will still keep a copy)
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

std::string Connection::TCB::to_string() const {
	ostringstream os;
	os << "SND"
		<< " .UNA = " << SND.UNA
		<< " .NXT = " << SND.NXT
		<< " .WND = " << SND.WND
		<< " .UP = " << SND.UP
		<< " .WL1 = " << SND.WL1
		<< " .WL2 = " << SND.WL2
		<< " ISS = " << ISS
		<< "\n RCV"
		<< " .NXT = " << RCV.NXT
		<< " .WND = " << RCV.WND
		<< " .UP = " << RCV.UP
		<< " IRS = " << IRS;
	return os.str();
}
