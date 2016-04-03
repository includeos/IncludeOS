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
#include <net/dhcp/dh4client.hpp>

const char* service_name__ = "TCP Test Service";

using namespace net;

using IPStack = Inet4<VirtioNet>;
std::unique_ptr<IPStack> inet;

//std::unique_ptr<TCP::Connection> server;
using namespace std::chrono;
void Service::start() {
  hw::Nic<VirtioNet>& eth0 = hw::Dev::eth<0,VirtioNet>();
  inet = std::make_unique<IPStack>(eth0);
  
  inet->network_config( {{ 10,0,0,42 }},      // IP
			{{ 255,255,255,0 }},  // Netmask
			{{ 10,0,0,1 }},       // Gateway
			{{ 8,8,8,8 }} );      // DNS
  
  
  auto& server = inet->tcp().bind(80);
  
  hw::PIT::instance().onTimeout(5s, [server]{
      printf("Server is running: %s \n", server.to_string().c_str());
    });

  server.onPacketReceived([](auto conn, auto packet) {
      printf("<Server> Received: %s\n", packet->to_string().c_str());

    }).onPacketDropped([](auto packet, std::string reason) {
	printf("<Server> Dropped: %s - Reason: %s \n", packet->to_string().c_str(), reason.c_str());

      }).onReceive([](auto conn, bool) {
	  conn->write("<html>Hey</html>");
	}).onConnect([](auto conn) {
	    printf("<Server> Connected.\n");
	  });
}
