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

TCP::TCP(IPStack& inet) : inet_(inet) {

}

TCP::Connection& TCP::bind(Port port) {
	TCP::Socket local{inet_.ip_addr(), port};
}

/*
	Bind a new connection to a given Port with a Callback.
*/
void TCP::bind(Port port, Connection::SuccessCallback success) {
	bind(port).onSuccess(success);
}

/*
	Active open a new connection to the given remote.
*/
TCP::Connection& TCP::connect(Socket& remote) {

}

/*
	Active open a new connection to the given remote.
*/
void TCP::connect(Socket& remote, Connection::SuccessCallback) {

}

/*
	Receive packet from transport layer (IP).
*/
void TCP::bottom(Packet_ptr pckt_ptr) {
	// Translate into a TCP::Packet. This will be used inside the TCP-scope.
	auto packet = static_pointer_cast<TCP::Packet>(pckt_ptr);
	
	// Who's the receiver?
	packet.destination();

	// Who sent it?
	packet.source();
	
}

/*
	Show all connections for TCP as a string.

	Format:
	[Protocol][Recv][Send][Local][Remote][State]
*/
string TCP::status() const {
	// Write all connections in a cute list.

}

void TCP::transmit(TCP::Packet packet) {
	// Translate into a Packet_ptr and send away.
}
