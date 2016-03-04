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

std::string small, big, huge;
int S{150}, B{1500}, H{15000};
std::string TEST_STR {"1337"};

class Buffer {
public:
	size_t written;
	size_t read;
	char* data;
	const size_t size;
	
	inline Buffer(size_t length) :
		written(0), read(0), data(new char[length]), size(length)
	{}

	inline ~Buffer() {
		delete[] data;
	}

	std::string str() {
		return {data, size};
	}
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
	tcp.bind(1).onConnect([](Connection_ptr conn) {
		conn->onReceive([](Connection_ptr conn, bool) {
			printf("conn.read() == small: [%d]\n", conn->read() == small);
		});
		conn->write(small);
		conn->close();
	});
	/*
		TEST: 1 server should be bound.
	*/
	printf("tcp.openPorts() == 1: [%d]\n", tcp.openPorts() == 1);
	
	/*
		TEST: Send and receive big string.
	*/
	tcp.bind(2).onConnect([](Connection_ptr conn) {
		conn->onReceive([](Connection_ptr conn, bool) {
			auto response = conn->read();
			bool OK = (response == big);
			printf("conn.read() == big: [%d]\n", OK);
		});
		conn->write(big);
		conn->close();
	});

	/*
		TEST: Send and receive huge string.
	*/
	tcp.bind(3).onConnect([](Connection_ptr conn) {
		auto buffer = std::make_shared<Buffer>(huge.size());
		conn->onReceive([buffer](Connection_ptr conn, bool) {
			// if not all expected data is read
			if(buffer->read < huge.size())
				buffer->read += conn->read(buffer->data+buffer->read, conn->receive_buffer().data_size());
			printf("huge read: %u\n", buffer->read);
			//
			if(buffer->written < huge.size()) {
				buffer->written += conn->write(huge.data()+buffer->written, huge.size() - buffer->written);
				printf("huge written: %u\n", buffer->written);
			}
			// 
			if(buffer->read == huge.size()) {
				//int compare = strcmp(buffer->data, huge.data());
				//bool OK = (compare == 0);
				bool OK = (buffer->str() == huge);
				printf("conn.read() == huge: [%d]\n", OK);
				conn->close();
			}
		});
		buffer->written += conn->write(huge.data()+buffer->written, huge.size() - buffer->written);
	});


}
