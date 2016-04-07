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

//#define DEBUG // Debug supression

#include <os>
#include <list>
#include <net/inet4>
#include <vector>

using namespace std;
using namespace net;

std::unique_ptr<net::Inet4<VirtioNet> > inet;

void Service::start()
{
  // Assign an IP-address, using Hårek-mapping :-)
  auto& eth0 = hw::Dev::eth<0,VirtioNet>();
  auto& inet = *new net::Inet4<VirtioNet>(eth0, // Device
    {{ 10,0,0,42 }}, // IP
    {{ 255,255,0,0 }} );  // Netmask

  printf("Service IP address is %s\n", inet.ip_addr().str().c_str());
  
  // UDP
  const UDP::port_t port = 4242;
  auto& sock = inet.udp().bind(port);
  
  sock.on_read(
  [&sock] (UDP::addr_t addr, UDP::port_t port,
           const char* data, size_t len)
  {
    std::string strdata(data, len);
    CHECK(1, "Getting UDP data from %s:  %d -> %s",
              addr.str().c_str(), port, strdata.c_str());
    // send the same thing right back!
    sock.sendto(addr, port, data, len,
    [] {
      // sent
      
      INFO("UDP test", "SUCCESS");
    });
  });
  
  INFO("UDP test", "Listening on port %d\n", port);
}
