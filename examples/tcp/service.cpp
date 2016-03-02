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

#include <os>
#include <net/inet4>
#include <math.h>
#include <net/dhcp/dh4client.hpp>

using namespace std::chrono;

// An IP-stack object
std::unique_ptr<net::Inet4<VirtioNet> > inet;
net::TCP::Socket python_server{ {{10,0,2,2}} , 1337};

void Service::start() {
  // Assign a driver (VirtioNet) to a network interface (eth0)
  hw::Nic<VirtioNet>& eth0 = hw::Dev::eth<0,VirtioNet>();
  
  // Bring up a network stack, attached to the nic
  inet = std::make_unique<net::Inet4<VirtioNet> >(eth0);
  
  // Static IP configuration, until we (possibly) get DHCP
  inet->network_config( {{ 10,0,0,42 }},      // IP
			{{ 255,255,255,0 }},  // Netmask
			{{ 10,0,0,1 }},       // Gateway
			{{ 8,8,8,8 }} );      // DNS
    
  
  // Set up a TCP server on port 80
  auto& server = inet->tcp().bind(80);
  
  printf("Server listening: %s \n", server.to_string().c_str());
  
  server
  // Client has connected
  .onConnect([](std::shared_ptr<net::TCP::Connection> client) {
  	printf("Connected [Client]: %s\n", client->remote().to_string().c_str());
		// Connect to our python server
		inet->tcp().connect(python_server)
		// When connection established
		->onConnect([client](std::shared_ptr<net::TCP::Connection> python) {
			printf("Connected [Python]: %s\n", python->to_string().c_str());
			
			client->onReceive([python](std::shared_ptr<net::TCP::Connection> client, bool) {
				// Read the request from our client
    		std::string request = client->read(1024);
    		printf("Received [Client]: %s\n", request.c_str());
    		python->write(request);

			});

			python->onReceive([client](std::shared_ptr<net::TCP::Connection> python, bool) {
				// Read the response from our python server
				std::string response = python->read(1024);
				client->write(response);

			});
		}); // << onConnect (python)
	}); // << onConnect (client)

}
