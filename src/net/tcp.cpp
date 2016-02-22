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

using namespace std;
using namespace net;


TCP::TCP(IPStack& inet) : 
	inet_(inet),
	listeners(),
	connections(),
	MAX_SEG_LIFETIME(30s)
{

}

/*
	Note: There is different approaches to how to handle listeners & connections.
	Need to discuss and decide for the best one.

	Best solution(?): 
	Preallocate a pool with listening connections. 
	When threshold is reach, remove/add new ones, similar to TCP window.

	Current solution:
	Simple.
*/
TCP::Connection& TCP::bind(Port port) {
	auto listen_conn_it = listeners.find(port);
	// Already a listening socket.
	if(listen_conn_it != listeners.end()) {
		throw TCPException{"Port is already taken."};
	}
	auto& connection = (listeners.emplace(port, Connection{*this, port})).first->second;
	printf("<TCP::bind> Bound to port %i \n", port);
	connection.open(false);
	return connection;
}

/*
	Active open a new connection to the given remote.

	@WARNING: Callback is added when returned (TCP::connect(...).onSuccess(...)), 
	and open() is called before callback is added.
*/
std::shared_ptr<TCP::Connection> TCP::connect(Socket remote) {
	std::shared_ptr<Connection> connection = add_connection(free_port(), remote);
	connection->open(true);
	return connection;
}

/*
	Active open a new connection to the given remote.
*/
void TCP::connect(Socket remote, Connection::ConnectCallback callback) {
	auto connection = add_connection(free_port(), remote);
	connection->onConnect(callback).open(true);
}

TCP::Seq TCP::generate_iss() {
	// Do something to get a iss.
	return rand();
}

TCP::Port TCP::free_port() {
	if(++current_ephemeral_ == 0)
		current_ephemeral_ = 1025;
	// Avoid giving a port that is bound to a service.
	while(listeners.find(current_ephemeral_) != listeners.end())
		current_ephemeral_++;

	return current_ephemeral_;
}


uint16_t TCP::checksum(TCP::Packet_ptr packet) {
	// TCP header
	TCP::Header* tcp_hdr = &(packet->header());
	// Pseudo header
	TCP::Pseudo_header pseudo_hdr;

	int tcp_length = packet->tcp_length();

	pseudo_hdr.saddr.whole = packet->src().whole;
	pseudo_hdr.daddr.whole = packet->dst().whole;
	pseudo_hdr.zero = 0;
	pseudo_hdr.proto = IP4::IP4_TCP;	 
	pseudo_hdr.tcp_length = htons(tcp_length);

	union {
		uint32_t whole;
		uint16_t part[2];
	} sum;

	sum.whole = 0;

	// Compute sum of pseudo header
	for (uint16_t* it = (uint16_t*)&pseudo_hdr; it < (uint16_t*)&pseudo_hdr + sizeof(pseudo_hdr)/2; it++)
		sum.whole += *it;       

	// Compute sum sum the actual header and data
	for (uint16_t* it = (uint16_t*)tcp_hdr; it < (uint16_t*)tcp_hdr + tcp_length/2; it++)
		sum.whole+= *it;

	// The odd-numbered case
	if (tcp_length & 1) {
		debug("<TCP::checksum> ODD number of bytes. 0-pading \n");
		union {
			uint16_t whole;
			uint8_t part[2];
		} last_chunk;
		last_chunk.part[0] = ((uint8_t*)tcp_hdr)[tcp_length - 1];
		last_chunk.part[1] = 0;
		sum.whole += last_chunk.whole;
	}

	debug2("<TCP::checksum: sum: 0x%x, half+half: 0x%x, TCP checksum: 0x%x, TCP checksum big-endian: 0x%x \n",
	 sum.whole, sum.part[0] + sum.part[1], (uint16_t)~((uint16_t)(sum.part[0] + sum.part[1])), htons((uint16_t)~((uint16_t)(sum.part[0] + sum.part[1]))));

	return ~(sum.part[0] + sum.part[1]);
}

void TCP::bottom(net::Packet_ptr packet_ptr) {
	// Translate into a TCP::Packet. This will be used inside the TCP-scope.
	auto packet = std::static_pointer_cast<TCP::Packet>(packet_ptr);
	printf("<TCP::bottom> TCP Packet received - Source: %s, Destination: %s \n", 
			packet->source().to_string().c_str(), packet->destination().to_string().c_str());
	
	// Do checksum
	if(checksum(packet)) {
		printf("<TCP::bottom> TCP Packet Checksum != 0 \n");
	}

	Connection::Tuple tuple { packet->dst_port(), packet->source() };

	// Try to find the receiver
	auto conn_it = connections.find(tuple);
	// Connection found
	if(conn_it != connections.end()) {
		printf("<TCP::bottom> Connection found: %s \n", conn_it->second->to_string().c_str());
		conn_it->second->receive(packet);
	}
	// No connection found 
	else {
		// Is there a listener?
		auto listen_conn_it = listeners.find(packet->dst_port());
		printf("<TCP::bottom> No connection found - looking for listener..\n");
		// Listener found => Create listening Connection
		if(listen_conn_it != listeners.end()) {
			auto& listen_conn = listen_conn_it->second;
			printf("<TCP::bottom> Listener found: %s ...\n", listen_conn.to_string().c_str());
			auto connection = (connections.emplace(tuple, std::make_shared<Connection>(listen_conn)).first->second);
			// Set remote
			connection->set_remote(packet->source());
			printf("<TCP::bottom> ... Creating connection: %s \n", connection->to_string().c_str());
			// Change to listening state.
			//connection.open(); // already listening
			connection->receive(packet);
		}
		// No listener found
		else {
			drop(packet);
		}
	}
}

/*
	Show all connections for TCP as a string.

	Format:
	[Protocol][Recv][Send][Local][Remote][State]
*/
string TCP::status() const {
	// Write all connections in a cute list.
	stringstream ss;
	ss << "LISTENING SOCKETS:\n";
	for(auto listen_it : listeners) {
		ss << listen_it.second.to_string() << "\n";
	}
	ss << "\nCONNECTIONS:\n" <<  "Proto\tRecv\tSend\tIn\tOut\tLocal\t\t\tRemote\t\t\tState\n";
	for(auto con_it : connections) {
		auto c = *(con_it.second);
		ss << "tcp4\t" 
			<< c.receive_buffer().size() << "\t" << c.send_buffer().size() << "\t"
			<< c.bytes_received() << "\t" << c.bytes_transmitted() << "\t"
			<< c.local().to_string() << "\t\t" << c.remote().to_string() << "\t\t" 
			<< c.state().to_string() << "\n";
	}
	return ss.str();
}

/*TCP::Socket& TCP::add_listener(TCP::Socket&& socket) {

}*/

std::shared_ptr<TCP::Connection> TCP::add_connection(Port local_port, TCP::Socket remote) {
	return 	(connections.emplace(
				Connection::Tuple{ local_port, remote }, 
				std::make_shared<Connection>(*this, local_port, remote))
			).first->second;
}

void TCP::close_connection(TCP::Connection& conn) {
	printf("<TCP::close_connection> Closing connection: %s \n", conn.to_string().c_str());
	connections.erase(conn.tuple());
	printf("<TCP::close_connection> TCP Status: \n%s \n", status().c_str());
}

void TCP::drop(TCP::Packet_ptr packet) {
	printf("<TCP::drop> Packet was dropped - no recipient: %s \n", packet->destination().to_string().c_str());
}

void TCP::transmit(TCP::Packet_ptr packet) {
	// Translate into a net::Packet_ptr and send away.
	// Generate checksum.
	packet->set_checksum(TCP::checksum(packet));
	//packet->set_checksum(checksum(packet));
	_network_layer_out(packet);
}
