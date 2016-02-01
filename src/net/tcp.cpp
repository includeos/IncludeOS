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

using namespace std;
using namespace net;

TCP::TCP(IPStack& inet) : 
	inet_(inet),
	listeners(),
	connections()
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
	TCP::Socket local{inet_.ip_addr(), port};
	
	auto sock_it = find(listeners.begin(), listeners.end(), local);
	// Already a listening socket.
	if(sock_it != listeners.end()) {
		panic("Can't bind, already taken.");
	}
	listeners.push_back(local);

	auto& connection = add_connection(listeners.back(), Socket{});
	connection.open();
	return connection;
}

/*
	Bind a new connection to a given Port with a Callback.
*/
void TCP::bind(Port port, Connection::SuccessCallback success) {
	bind(port).onSuccess(success);
}

/*
	Active open a new connection to the given remote.

	@WARNING: Callback is added when returned (TCP::connect(...).onSuccess(...)), 
	and open() is called before callback is added.
*/
TCP::Connection& TCP::connect(Socket&& remote) {
	TCP::Socket local{inet_.ip_addr(), free_port()};
	TCP::Connection& connection = add_connection(local, forward<TCP::Socket>(remote));
	connection.open(true);
	return connection;
}

/*
	Active open a new connection to the given remote.
*/
void TCP::connect(Socket& remote, Connection::SuccessCallback callback) {
	TCP::Socket local{inet_.ip_addr(), free_port()};
	TCP::Connection& connection = add_connection(local, forward<TCP::Socket>(remote));
	connection.onSuccess(callback).open(true);
}

TCP::Seq generate_iss() {
	// Do something to get a iss.
	return rand();
}

TCP::Port free_port() {
	// Check what ports are taken, get a new random one.
	return 1337;
}


uint16_t TCP::checksum(TCP::Packet_ptr packet){  
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
	
	// Do checksum
	if(checksum(packet)) {
		// drop?
	}

	Connection::Tuple tuple { packet->destination(), packet->source() };

	// Try to find the receiver
	auto conn_it = connections.find(tuple);
	// Connection found
	if(conn_it != connections.end()) {
		conn_it->second.receive(packet);
	}
	// No connection found 
	else {
		// Is there a listener?
		auto sock_it = find(listeners.begin(), listeners.end(), packet->destination());
		// Listener found => Create listening Connection
		if(sock_it != listeners.end()) {
			auto connection = add_connection(*sock_it, packet->destination());
			// Change to listening state.
			connection.open();
			connection.receive(packet);
		}
		// No listener found
		else {
			drop(packet);
			return; // TODO: Remove silent return.
		}
	}
}

TCP::Packet_ptr TCP::net2tcp(net::Packet_ptr packet_ptr) {
	return std::static_pointer_cast<TCP::Packet>(packet_ptr);
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
	for(auto sock : listeners) {
		ss << sock.to_string() << "\n";
	}
	ss << "\nCONNECTIONS:\n" <<  "Proto\tRecv\tSend\tLocal\tRemote\tState\n";
	for(auto con_it : connections) {
		auto conn = con_it.second;
		ss << "tcp4\t0\t0\t" << conn.local().to_string() << "\t" << conn.remote().to_string() << "\t" << conn.state().to_string() << "\n";
	}
	return ss.str();
}

/*TCP::Socket& TCP::add_listener(TCP::Socket&& socket) {

}*/

TCP::Connection& TCP::add_connection(TCP::Socket& local, TCP::Socket&& remote) {
	return 	(connections.emplace(
				Connection::Tuple{ local, remote }, 
				Connection{*this, local, forward<TCP::Socket>(remote), 
				generate_iss()})
			).first->second;
}

void TCP::drop(TCP::Packet_ptr packet) {
	// Packet is dropped.
}

void TCP::transmit(TCP::Packet_ptr packet) {
	// Translate into a net::Packet_ptr and send away.
	// Generate checksum.
	packet->set_checksum(TCP::checksum(packet));
	//packet->set_checksum(checksum(packet));
	_network_layer_out(packet);
}
