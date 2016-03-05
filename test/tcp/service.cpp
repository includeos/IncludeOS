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
#include <info>

using namespace net;
using namespace std::chrono; // For timers and MSL
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

size_t bufstore_capacity{0};

// Default MSL is 30s. Timeout 2*MSL.
// To reduce test duration, lower MSL to 5s.
milliseconds MSL_TEST = 5s;

void FINISH_TEST() {
	INFO("TEST", "Started 3 x MSL timeout.");
	hw::PIT::instance().onTimeout(3 * MSL_TEST, [] {
		INFO("TEST", "Verify release of resources");
		CHECK(inet->tcp().activeConnections() == 0, "tcp.activeConnections() == 0");
		CHECK(inet->available_capacity() == bufstore_capacity, 
			"inet.available_capacity() == bufstore_capacity");
		printf("# TEST DONE #\n");
	});
}

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
 	
 	bufstore_capacity = inet->available_capacity();
 	auto& tcp = inet->tcp();
 	tcp.set_buffer_limit(10);
 	tcp.set_MSL(MSL_TEST);
	
	/*
		TEST: Send and receive small string.
	*/
	INFO("TEST", "Listeners and connections allocation.");
	CHECK(tcp.openPorts() == 0, "tcp.openPorts() == 0");
	CHECK(tcp.activeConnections() == 0, "tcp.activeConnections() == 0");
	
	tcp.bind(TEST1).onConnect([](Connection_ptr conn) {
		INFO("TEST", "SMALL string");
		conn->onReceive([](Connection_ptr conn, bool) {
			CHECK(conn->read() == small, "conn.read() == small");
			conn->close();
		});
		conn->write(small);
	});
	/*
		TEST: 1 server should be bound.
	*/
	CHECK(tcp.openPorts() == 1, "tcp.openPorts() == 1");
	
	/*
		TEST: Send and receive big string.
	*/
	tcp.bind(TEST2).onConnect([](Connection_ptr conn) {
		INFO("TEST", "BIG string");
		auto response = std::make_shared<std::string>();
		conn->onReceive([response](Connection_ptr conn, bool) {
			*response += conn->read();
			if(response->size() == big.size()) {
				bool OK = (*response == big);
				CHECK(OK, "conn.read() == big");
				conn->close();
			}
		});
		conn->write(big);
	});

	/*
		TEST: Send and receive huge string.
	*/
	tcp.bind(TEST3).onConnect([](Connection_ptr conn) {
		INFO("TEST", "HUGE string");
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
				CHECK(OK, "conn.read() == huge");
				conn->close();
			}
		});
		buffer->written += conn->write(huge.data(), huge.size());
	});

	CHECK(tcp.openPorts() == 3, "tcp.openPorts() == 3");

	/*
		Test for other stuff
	*/
	tcp.bind(TEST4).onConnect([](Connection_ptr conn) {
		INFO("TEST","Connection");
		// There should be at least one connection.
		CHECK(inet->tcp().activeConnections() > 0, "tcp.activeConnections() > 0");
		// Test if connected.
		CHECK(conn->is_connected(), "conn.is_connected()");
		// Test if writable.
		CHECK(conn->is_writable(), "conn.is_writable()");
		// Test if state is ESTABLISHED.
		CHECK(conn->is_state({"ESTABLISHED"}), "conn.is_state(ESTABLISHED)");

		INFO("TEST", "Active close");
		// Test for active close.
		conn->close();
		CHECK(!conn->is_writable(), "!conn->is_writable()");
		CHECK(conn->is_state({"FIN-WAIT-1"}), "conn.is_state(FIN-WAIT-1)");
	})
	.onDisconnect([](Connection_ptr conn, std::string) {
		CHECK(conn->is_state({"FIN-WAIT-2"}), "conn.is_state(FIN-WAIT-2)");
		hw::PIT::instance().onTimeout(1s,[conn]{
			CHECK(conn->is_state({"TIME-WAIT"}), "conn.is_state(TIME-WAIT)");
		});
		FINISH_TEST();
	});


}
