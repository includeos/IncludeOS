// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
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

#include <os>
#include <net/inet4>
#include <net/dhcp/dh4client.hpp>
#include <net/tcp.hpp>
#include <vector>

using namespace net;
using Connection_ptr = std::shared_ptr<TCP::Connection>;
std::unique_ptr<Inet4<VirtioNet>> inet;
std::shared_ptr<TCP::Connection> client;

/*
	TEST VARIABLES
*/
TCP::Port 
	TEST1{1}, TEST2{2}, TEST3{3}, TEST4{4};

std::string 
	small, big, huge;

int 
	S{150}, B{1500}, H{15000};

std::string 
	TEST_STR {"1337"};

struct Buffer {
	size_t written, read;
	char* data;
	const size_t size;
	
	Buffer(size_t length) :
		written(0), read(0), data(new char[length]), size(length) {}

	~Buffer() { delete[] data; }

	std::string str() { return {data, size};}
};

void Service::start()
{
	for(int i = 0; i < S; i++) small += TEST_STR;
	for(int i = 0; i < B; i++) big += TEST_STR;		
	for(int i = 0; i < H; i++) huge += TEST_STR;

	hw::Nic<VirtioNet>& eth0 = hw::Dev::eth<0,VirtioNet>();
  inet = std::make_unique<Inet4<VirtioNet>>(eth0);
  
  inet->network_config( {{ 10,0,0,42 }},      // IP
			{{ 255,255,255,0 }},  // Netmask
			{{ 10,0,0,1 }},       // Gateway
			{{ 8,8,8,8 }} );      // DNS
 	
 	auto& tcp = inet->tcp();
 	tcp.set_buffer_limit(10);
	
	/*
		TEST: Send and receive small string.
	*/
	printf("# TEST BIND PORT #\n");
	printf("[%d] : tcp.openPorts() == 0\n", tcp.openPorts() == 0);
	printf("[%d] : tcp.activeConnections() == 0\n", tcp.activeConnections() == 0);
	tcp.bind(TEST1).onConnect([](Connection_ptr conn) {
		printf("# TEST SMALL STRING #\n");
		conn->onReceive([](Connection_ptr conn, bool) {
			printf("[%d] : conn.read() == small\n", conn->read() == small);
			conn->close();
		});
		conn->write(small);
	});
	/*
		TEST: 1 server should be bound.
	*/
	printf("[%d] : tcp.openPorts() == 1\n", tcp.openPorts() == 1);
	
	/*
		TEST: Send and receive big string.
	*/
	tcp.bind(TEST2).onConnect([](Connection_ptr conn) {
		printf("# TEST BIG STRING #\n");
		auto response = std::make_shared<std::string>();
		conn->onReceive([response](Connection_ptr conn, bool) {
			*response += conn->read();
			if(response->size() == big.size()) {
				bool OK = (*response == big);
				printf("[%d] : conn.read() == big\n", OK);
				conn->close();
			}
		});
		conn->write(big);
	});

	/*
		TEST: Send and receive huge string.
	*/
	tcp.bind(TEST3).onConnect([](Connection_ptr conn) {
		printf("# TEST HUGE STRING #\n");
		auto buffer = std::make_shared<Buffer>(huge.size());
		conn->onReceive([buffer](Connection_ptr conn, bool) {
			// if not all expected data is read
			if(buffer->read < huge.size())
				buffer->read += conn->read(buffer->data+buffer->read, conn->receive_buffer().data_size());
			// if not all expected data is written
			if(buffer->written < huge.size()) {
				buffer->written += conn->write(huge.data()+buffer->written, huge.size() - buffer->written);
			}
			// when all expected data is read
			if(buffer->read == huge.size()) {
				bool OK = (buffer->str() == huge);
				printf("[%d] : conn.read() == huge\n", OK);
				conn->close();
			}
		});
		buffer->written += conn->write(huge.data(), huge.size());
	});

	printf("[%d] : tcp.openPorts() == 3\n", tcp.openPorts() == 3);

	/*
		Test for other stuff
	*/
	tcp.bind(TEST4).onConnect([](Connection_ptr conn) {
		printf("# TEST CONNECTION #\n");
		// There should be at least one connection.
		printf("[%d] : tcp.activeConnections() > 0\n", inet->tcp().activeConnections() > 0);
		// Test if connected.
		printf("[%d] : conn.is_connected()\n", conn->is_connected());
		// Test if writable.
		printf("[%d] : conn.is_writable()\n", conn->is_writable());
		// Test if state is ESTABLISHED.
		printf("[%d] : conn.is_state(ESTABLISHED)\n", conn->is_state("ESTABLISHED"));

		printf("# TEST ACTIVE CLOSE #\n");
		// Test for active close.
		conn->close();
		printf("[%d] : !conn.is_writable()\n", !conn->is_writable());
		printf("[%d] : conn.is_state(FIN-WAIT-1) \n", conn->is_state("FIN-WAIT-1"));
	})
	.onDisconnect([](Connection_ptr conn, std::string) {
		printf("[%d] : conn.is_state(FIN-WAIT-2) \n", conn->is_state("FIN-WAIT-2"));
		using namespace std::chrono;
		hw::PIT::instance().onTimeout(1s,[conn]{
			printf("[%d] : conn.is_state(TIME-WAIT)\n", conn->is_state("TIME-WAIT"));
		});
	});


}
