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

using namespace net;
using Connection_ptr = std::shared_ptr<TCP::Connection>;
std::unique_ptr<Inet4<VirtioNet>> inet;
std::shared_ptr<TCP::Connection> client;

std::string small, big, huge;
int S{150}, B{1500}, H{15000};
std::string TEST_STR {"1337"};

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
		conn->onReceive([](Connection_ptr conn, bool) {
			auto response = conn->read();
			bool OK = (response == huge);
			printf("conn.read() == huge: [%d]\n", OK);
		});
		conn->write(huge);
		conn->close();
	});


}
