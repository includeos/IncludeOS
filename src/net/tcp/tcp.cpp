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

TCP::TCP(IPStack& inet) : 
	inet_(inet),
	listeners(),
	connections(), 
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
	
	auto listener = listener.find(local);
	// Already a listening socket.
	if(listener != listener.end()) {
		panic("Can't bind, already taken.");
	}

	listener = listeners.push_back(local);

	Connection conn{*this, &listener, generate_iss()};
	auto& connection = (connections.emplace_back({conn.tuple(),conn}))->second;
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
TCP::Connection& TCP::connect(Socket& remote) {
	TCP::Socket local{inet_.ip_addr(), generate_port()};

	Connection conn{*this, local, remote, generate_iss()};
	auto& connection = (connections.emplace_back({conn.tuple(), conn}))->second;
	connection.open(true);
	return connection;
}

/*
	Active open a new connection to the given remote.
*/
void TCP::connect(Socket& remote, Connection::SuccessCallback callback) {
	TCP::Socket local{inet_.ip_addr(), generate_port()};

	Connection conn{*this, local, remote, generate_iss()};
	auto& connection = (connections.emplace_back({conn.tuple(), conn}))->second;
	connection.onSuccess(callback).open(true);
	return connection;
}

TCP::Seq generate_iss() {
	// Do something to get a iss.
	return 42;
}

TCP::Port generate_port() {
	// Check what ports are taken, get a new random one.
	return 1337;
}

/*
	Receive packet from transport layer (IP).
*/
void TCP::bottom(net::Packet_ptr pckt_ptr) {
	// Translate into a TCP::Packet. This will be used inside the TCP-scope.
	auto packet = std::static_pointer_cast<TCP::Packet>(local_stack_.createPacket(sizeof(pckt_ptr)));
	
	// Who's the receiver?
	packet->destination();

	// Who sent it?
	packet->source();
	
}

/*
	Show all connections for TCP as a string.

	Format:
	[Protocol][Recv][Send][Local][Remote][State]
*/
string TCP::status() const {
	// Write all connections in a cute list.

}

void TCP::transmit(TCP::Packet_ptr packet) {
	// Translate into a net::Packet_ptr and send away.
	auto pckt_ptr;
	inet_.transmit(pckt_ptr);
}
