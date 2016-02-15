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
#include <math.h>
#include <iostream>
#include <sstream>
#include <net/dhcp/dh4client.hpp>

using namespace std::chrono;

// An IP-stack object
std::unique_ptr<net::Inet4<VirtioNet> > inet;

void Service::start() {
  
  // Assign a driver (VirtioNet) to a network interface (eth0)
  // @note: We could determine the appropirate driver dynamically, but then we'd
  // have to include all the drivers into the image, which  we want to avoid.
  Nic<VirtioNet>& eth0 = Dev::eth<0,VirtioNet>();
  
  // Bring up a network stack, attached to the nic
  // @note : No parameters after 'nic' means we'll use DHCP for IP config.
  inet = std::make_unique<net::Inet4<VirtioNet> >(eth0);
  
  // Static IP configuration, until we (possibly) get DHCP
  // @note : Mostly to get a robust demo service that it works with and without DHCP
  inet->network_config( {{ 10,0,0,42 }},      // IP
			{{ 255,255,255,0 }},  // Netmask
			{{ 10,0,0,1 }},       // Gateway
			{{ 8,8,8,8 }} );      // DNS
  
  printf("Size of IP-stack: %i b \n",sizeof(inet));
  printf("Service IP address: %s \n", inet->ip_addr().str().c_str());
  
  // Set up a TCP server on port 80
  net::TCP::Connection& conn =  inet->tcp().bind(80);

  
  srand(OS::cycles_since_boot());
  
  printf("<Service> Connection: %s \n", conn.to_string().c_str());
  // Add a TCP connection handler - here a hardcoded HTTP-service
  conn.onData([](net::TCP::Connection& conn, bool push) {
      std::string data = conn.read(1024);
      printf("<Service> onData: \n %s \n", data.c_str());
      printf("<Service> TCP STATUS:\n%s \n", conn.host().status().c_str());
      std::stringstream ss;
      for(int i = 0; i < 1500; i++) {
        ss << (char)i;
      }
      std::string output{"HTTP/1.1 200 OK \n\n <html>" + ss.str() + "</html>"};
      conn.write(output.data(), output.size());
      //conn.close();
  }).onDisconnect([](net::TCP::Connection& conn, std::string msg) {
      printf("<Service> onDisconnect: %s \n", msg.c_str());
      printf("<Service> TCP STATUS:\n%s", conn.host().status().c_str());
  });

  printf("*** TEST SERVICE STARTED *** \n");

}
