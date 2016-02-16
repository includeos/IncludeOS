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
	receive_buffer_(),
	send_buffer_(),
	time_wait_started(0)
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
	receive_buffer_(),
	send_buffer_(),
	time_wait_started(0)
{
	
}


size_t Connection::read(char* buffer, size_t n) {
	printf("<TCP::Connection::read> Reading %u bytes of data from RCV buffer. Total amount of packets stored: %u\n", 
			n, receive_buffer().size());
	return state_->receive(*this, buffer, n);
}

// TODO: The n == 0 part.
std::string Connection::read(size_t n) {
	char buffer[n];
	size_t length = read(&buffer[0], n);
	return {buffer, length};
}

size_t Connection::read_from_receive_buffer(char* buffer, size_t n) {
	size_t bytes_read = 0;
	// Read data to buffer until either whole buffer is emptied, or the user got all the data requested.
	while(!receive_buffer().empty() and bytes_read < n)
	{
		// Packet in front
		auto packet = receive_buffer().front();
		// Where to begin reading
		char* begin = packet->data()+rcv_buffer_offset;
		// Read this iteration
		size_t total{0};
		// Remaining bytes to read.
		size_t remaining = n - bytes_read;
		// Trying to read over more than one packet
		if( remaining >= (packet->data_length() - rcv_buffer_offset) ) {
			// Reading whole packet
			total = packet->data_length();
			// Removing packet from receive buffer.
			receive_buffer().pop();
			// Next packet will start from beginning.
			rcv_buffer_offset = 0;
		}
		// Reading less than one packet.
		else {
			total = remaining;
			rcv_buffer_offset = packet->data_length() - remaining;
		}
		memcpy(buffer, begin, total);
		bytes_read += total;
	}

	return bytes_read;
}

bool Connection::add_to_receive_buffer(TCP::Packet_ptr packet) {
	// TODO: Check for buffer limit.
	receive_buffer().push(packet);
	return true;
}

size_t Connection::write(const char* buffer, size_t n) {
	printf("<TCP::Connection::write> Writing %u bytes of data to SND buffer. \n", n);
	return state_->send(*this, buffer, n);
}

size_t Connection::write_to_send_buffer(const char* buffer, size_t n, bool PUSH) {
	size_t bytes_written{0};
	size_t remaining{n};
	do {
		auto packet = create_outgoing_packet();
		size_t written = packet->set_seq(control_block.SND.NXT).set_ack(control_block.RCV.NXT).set_flag(ACK).fill(buffer + (n-remaining), remaining);
		bytes_written += written;
		remaining -= written;
		
		printf("<TCP::Connection::write_to_send_buffer> Remaining: %u \n", remaining);
		
		// If last packet, add PUSH.
		if(!remaining and PUSH)
			packet->set_flag(PSH);

		// Advance outgoing sequence number (SND.NXT) with the length of the data.
		control_block.SND.NXT += packet->data_length();
	} while(remaining);

	return bytes_written;
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
	printf("<TCP::Connection::close> Active close on connection. \n");
	state_->close(*this);
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
	switch(state_->handle(*this, incoming)) {
		case State::CLOSED: {
			printf("<TCP::Connection::receive> State handle finished with CLOSED. We're done, ask host() to delete the connection. \n");
			signal_close();
			break;
		};
		case State::CLOSE: {
			printf("<TCP::Connection::receive> State handle finished with CLOSE. onDisconnect has been called, close the connection. \n");
			state_->close(*this);
			break;
		};
	}
	
	
}

Connection::~Connection() {
	// Do all necessary clean up.
	// Free up buffers etc.
	printf("<TCP::Connection::~Connection> Bye bye... \n");
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

	// Will also add the packet to the back of the send queue.
	send_buffer().push(packet);

	return packet;
}

void Connection::transmit() {
	assert(! send_buffer().empty() );
	
	printf("<TCP::Connection::transmit> Transmitting: [ %i ] packets. \n", send_buffer().size());	
	while(! send_buffer_.empty() ) {
		auto packet = send_buffer().front();
		assert(! packet->destination().is_empty());
		transmit(packet);
		send_buffer().pop();
	}
}

void Connection::transmit(TCP::Packet_ptr packet) {
	printf("<TCP::Connection::transmit> Transmitting: %s \n", packet->to_string().c_str());
	host_.transmit(packet);
	// Don't think we would like to retransmit reset packets..?
	if(!packet->isset(RST))
		add_retransmission(packet);
}

TCP::Packet_ptr Connection::outgoing_packet() {
	if(send_buffer().empty())
		create_outgoing_packet();
	return send_buffer().back();
}

TCP::Seq Connection::generate_iss() {
	return host_.generate_iss();
}

void Connection::add_retransmission(TCP::Packet_ptr packet) {
	printf("<TCP::Connection::add_retransmission> Packet added to retransmission. \n");
	hw::PIT::instance().onTimeout(RTO(), [packet, this] {
		// Packet hasnt been ACKed.
		if(packet->seq() > tcb().SND.UNA) {
			printf("<TCP::Connection::add_retransmission@onTimeout> Packet unacknowledge, retransmitting...\n");
			transmit(packet);
		} else {
			debug2("<TCP::Connection::add_retransmission@onTimeout> Packet acknowledged %s \n", packet->to_string().c_str());
		}
	});
}
/*
	Next compute a Smoothed Round Trip Time (SRTT) as:

    SRTT = ( ALPHA * SRTT ) + ((1-ALPHA) * RTT)

	and based on this, compute the retransmission timeout (RTO) as:

	RTO = min[UBOUND,max[LBOUND,(BETA*SRTT)]]

	where UBOUND is an upper bound on the timeout (e.g., 1 minute),
	LBOUND is a lower bound on the timeout (e.g., 1 second), ALPHA is
	a smoothing factor (e.g., .8 to .9), and BETA is a delay variance
	factor (e.g., 1.3 to 2.0).
*/
std::chrono::milliseconds Connection::RTO() const {
	return 1s;
}

void Connection::start_time_wait_timeout() {
	printf("<TCP::Connection::start_time_wait_timeout> Time Wait timer started. \n");
	time_wait_started = OS::cycles_since_boot();
	//auto timeout = 2 * host().MSL(); // 60 seconds
	auto timeout = 10s;
	hw::PIT::instance().onTimeout(timeout,[this, timeout] {
		// The timer hasnt been updated
		if( OS::cycles_since_boot() >= (time_wait_started + timeout.count()) ) {
			signal_close();
		} else {
			printf("<TCP::Connection::start_time_wait_timeout> time_wait_started has been updated. \n");
		}
	});
}

void Connection::signal_close() {
	printf("<TCP::Connection::signal_close> It's time to delete this connection. \n");
	host_.close_connection(*this);
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
