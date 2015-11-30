// Part of the IncludeOS Unikernel  - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and  Alfred Bratterud. 
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


//#define DEBUG // Debug supression

#include <os>
#include <list>
#include <net/inet4>
#include <vector>

using namespace std;
using namespace net;

void Service::start()
{
  // Assign an IP-address, using Hårek-mapping :-)
  auto& eth0 = Dev::eth<0,VirtioNet>();
  auto& mac = eth0.mac(); 
  
  auto& inet = *new net::Inet4<VirtioNet>(eth0, // Device
    {{ mac.part[2],mac.part[3],mac.part[4],mac.part[5] }}, // IP
    {{ 255,255,0,0 }} );  // Netmask
  
  printf("Service IP address: %s \n", inet.ip_addr().str().c_str());
  
  // UDP
  UDP::port port = 4242;
  auto& sock = inet.udp().bind(port);
  
  sock.onRead(
  [] (SocketUDP& conn, UDP::addr_t addr, UDP::port port, const std::string& data) -> int
  {
    printf("Getting UDP data from %s: %i: %s\n", 
          addr.str().c_str(), port, data.c_str());
    // send the same thing right back!
    conn.write(addr, port, data);
    return 0;
  });
  
  printf("UDP server listening to port %i \n",port);
  
}
