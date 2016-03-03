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
#define DEBUG
#define DEBUG2

#include <net/tcp.hpp>
#include <net/tcp_connection_states.hpp>

using namespace net;
using Connection = TCP::Connection;

using namespace std;

/*
	This is most likely used in a ACTIVE open
*/
Connection::Connection(TCP& host, Port local_port, Socket remote) :
	host_(host),
	local_port_(local_port), 
	remote_(remote),
	state_(&Connection::Closed::instance()),
	prev_state_(state_),
	control_block(),
	receive_buffer_(host.buffer_limit()),
	send_buffer_(host.buffer_limit()),
	time_wait_started(0)
{

}

/*
	This is most likely used in a PASSIVE open
*/
Connection::Connection(TCP& host, Port local_port) :
	host_(host),
	local_port_(local_port), 
	remote_(TCP::Socket()),
	state_(&Connection::Closed::instance()),
	prev_state_(state_),
	control_block(),
	receive_buffer_(host.buffer_limit()),
	send_buffer_(host.buffer_limit()),
	time_wait_started(0)
{
	
}


size_t Connection::read(char* buffer, size_t n) {
	debug("<TCP::Connection::read> Reading %u bytes of data from RCV buffer. Total amount of packets stored: %u\n", 
			n, receive_buffer_.size());
	try {
		return state_->receive(*this, buffer, n);	
	} catch(TCPException err) {
		signal_error(err);
		return 0;
	}
}

std::string Connection::read(size_t n) {
	if(n == 0) {
		// Read all data.
		n = receive_buffer_.data_size();
	}
	char buffer[n];
	size_t length = read(&buffer[0], n);
	return {buffer, length};
}

size_t Connection::read_from_receive_buffer(char* buffer, size_t n) {
	size_t bytes_read = 0;
	// Read data to buffer until either whole buffer is emptied, or the user got all the data requested.
	while(!receive_buffer_.empty() and bytes_read < n)
	{
		// Packet in front
		auto packet = receive_buffer_.front();
		// Where to begin reading
		char* begin = packet->data()+receive_buffer_.data_offset();
		// Read this iteration
		size_t total{0};
		// Remaining bytes to read.
		size_t remaining = n - bytes_read;
		// Trying to read over more than one packet
		if( remaining >= (packet->data_length() - receive_buffer_.data_offset()) ) {
			debug2("<TCP::Connection_read_from_receive_buffer> Remaining >: %u Current p: %u\n", 
				remaining, packet->data_length() - receive_buffer_.data_offset());
			// Reading whole packet
			total = packet->data_length();
			// Removing packet from receive buffer.
			receive_buffer_.pop();
			// Next packet will start from beginning.
			receive_buffer_.set_data_offset(0);
		}
		// Reading less than one packet.
		else {
			debug2("<TCP::Connection_read_from_receive_buffer> Remaining <: %u\n", remaining);
			total = remaining;
			receive_buffer_.set_data_offset(packet->data_length() - remaining);
		}
		memcpy(buffer+bytes_read, begin, total);
		bytes_read += total;
	}

	return bytes_read;
}

bool Connection::add_to_receive_buffer(TCP::Packet_ptr packet) {
	return receive_buffer_.add(packet);
}

size_t Connection::write(const char* buffer, size_t n, bool PUSH) {
	debug("<TCP::Connection::write> Asking to write %u bytes of data to SND buffer. \n", n);
	try {
		return state_->send(*this, buffer, n, PUSH);
	} catch(TCPException err) {
		signal_error(err);
		return 0;
	}
}

size_t Connection::write_to_send_buffer(const char* buffer, size_t n, bool PUSH) {
	size_t bytes_written{0};
	size_t remaining{n};
	do {
		auto packet = create_outgoing_packet();
		size_t written = packet->set_seq(control_block.SND.NXT).set_ack(control_block.RCV.NXT).set_flag(ACK).fill(buffer + (n-remaining), remaining);
		bytes_written += written;
		remaining -= written;
		
		debug("<TCP::Connection::write_to_send_buffer> Written: %u - Remaining: %u \n", written, remaining);
		
		// If last packet, add PUSH.
		if(!remaining and PUSH)
			packet->set_flag(PSH);

		// Advance outgoing sequence number (SND.NXT) with the length of the data.
		control_block.SND.NXT += packet->data_length();
	} while(remaining and !send_buffer_.full());

	return bytes_written;
}

/*
	If ACTIVE: 
	Need a remote Socket.
*/
void Connection::open(bool active) {
	try {
		debug("<TCP::Connection::open> Trying to open Connection...\n");
		state_->open(*this, active);
	}
	// No remote host, or state isnt valid for opening.
	catch (TCPException e) {
		debug("<TCP::Connection::open> Cannot open Connection. \n");
		signal_error(e);
	}	
}

void Connection::close() {
	debug("<TCP::Connection::close> Active close on connection. \n");
	try {
		state_->close(*this);
		if(is_state(Closed::instance()))
			signal_close();
	} catch(TCPException err) {
		signal_error(err);
	}
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
	signal_packet_received(incoming);
	// Change window accordingly. 
	control_block.SND.WND = incoming->win();
	switch(state_->handle(*this, incoming)) {
		case State::OK: {
			// Do nothing.
			break;
		}
		case State::CLOSED: {
			debug("<TCP::Connection::receive> State handle finished with CLOSED. We're done, ask host() to delete the connection. \n");
			signal_close();
			break;
		};
		case State::CLOSE: {
			debug("<TCP::Connection::receive> State handle finished with CLOSE. onDisconnect has been called, close the connection. \n");
			state_->close(*this);
			break;
		};
	}
}

bool Connection::is_listening() const { 
	return is_state(Listen::instance()); 
}

bool Connection::is_connected() const { 
	return is_state(Established::instance()); 
}

bool Connection::is_closing() const { 
	return (is_state(Closing::instance()) or is_state(LastAck::instance()) or is_state(TimeWait::instance())); 
}

bool Connection::is_writable() const {
	return (is_connected() and (!send_buffer_.full()));
}

Connection::~Connection() {
	// Do all necessary clean up.
	// Free up buffers etc.
	debug2("<TCP::Connection::~Connection> Bye bye... \n");
}


TCP::Packet_ptr Connection::create_outgoing_packet() {
	auto packet = std::static_pointer_cast<TCP::Packet>((host_.inet_).createPacket(TCP::Packet::HEADERS_SIZE));
	
	packet->init();
	// Set Source (local == the current connection)
	packet->set_source(local());
	// Set Destination (remote)
	packet->set_destination(remote_);

	packet->set_win_size(control_block.SND.WND);
	
	// Set SEQ and ACK - I think this is OK..
	packet->set_seq(control_block.SND.NXT).set_ack(control_block.RCV.NXT);
	debug("<TCP::Connection::create_outgoing_packet> Outgoing packet created: %s \n", packet->to_string().c_str());

	// Will also add the packet to the back of the send queue.
	send_buffer_.push(packet);

	return packet;
}

void Connection::transmit() {
	assert(! send_buffer_.empty() );
	
	debug("<TCP::Connection::transmit> Transmitting: [ %i ] packets. \n", send_buffer_.size());	
	while(! send_buffer_.empty() ) {
		auto packet = send_buffer_.front();
		assert(! packet->destination().is_empty());
		transmit(packet);
		send_buffer_.pop();
	}
}

void Connection::transmit(TCP::Packet_ptr packet) {
	debug("<TCP::Connection::transmit> Transmitting: %s \n", packet->to_string().c_str());
	host_.transmit(packet);
	// Don't think we would like to retransmit reset packets..?
	//if(!packet->isset(RST))
	//	add_retransmission(packet);
}

TCP::Packet_ptr Connection::outgoing_packet() {
	if(send_buffer_.empty())
		create_outgoing_packet();
	return send_buffer_.back();
}

TCP::Seq Connection::generate_iss() {
	return host_.generate_iss();
}

void Connection::set_state(State& state) {
	prev_state_ = state_;
	state_ = &state;
	debug("<TCP::Connection::set_state> %s => %s \n", 
			prev_state_->to_string().c_str(), state_->to_string().c_str());
}

void Connection::add_retransmission(TCP::Packet_ptr packet) {
	debug2("<TCP::Connection::add_retransmission> Packet added to retransmission. \n");
	auto self = shared_from_this();
	hw::PIT::instance().onTimeout(RTO(), [packet, self] {
		// Packet hasnt been ACKed.
		if(packet->seq() > self->tcb().SND.UNA) {
			debug("<TCP::Connection::add_retransmission@onTimeout> Packet unacknowledge, retransmitting...\n");
			self->transmit(packet);
		} else {
			debug2("<TCP::Connection::add_retransmission@onTimeout> Packet acknowledged %s \n", packet->to_string().c_str());
			// Signal user?
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
	debug2("<TCP::Connection::start_time_wait_timeout> Time Wait timer started. \n");
	time_wait_started = OS::cycles_since_boot();
	//auto timeout = 2 * host().MSL(); // 60 seconds
	auto timeout = 10s;
	// Passing "this"..?
	hw::PIT::instance().onTimeout(timeout,[this, timeout] {
		// The timer hasnt been updated
		if( OS::cycles_since_boot() >= (time_wait_started + timeout.count()) ) {
			signal_close();
		} else {
			debug2("<TCP::Connection::start_time_wait_timeout> time_wait_started has been updated. \n");
		}
	});
}

void Connection::signal_close() {
	debug("<TCP::Connection::signal_close> It's time to delete this connection. \n");
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
