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
#include <timers>

using namespace net;
using namespace std::chrono;

void Service::start(const std::string&)
{
  auto& inet = Inet4::stack<0>();
  inet.network_config({  10,  0,  0, 45 },   // IP
                      { 255, 255, 0,  0 },   // Netmask
                      {  10,  0,  0,  1 } ); // Gateway
  printf("Service IP address is %s\n", inet.ip_addr().str().c_str());

  // UDP
  const UDP::port_t port = 4242;
  auto& sock = inet.udp().bind(port);

  sock.on_read([&sock] (UDP::addr_t addr, UDP::port_t port,
                        const char* data, size_t len) {
                 std::string strdata(data, len);
                 CHECK(1, "Getting UDP data from %s:  %d -> %s",
                       addr.str().c_str(), port, strdata.c_str());
                 // send the same thing right back!
                 Timers::oneshot(100ms, [&sock, addr, port, data, len](Timers::id_t){
                     sock.sendto(addr, port, data, len);
                   });
               });

  INFO("UDP test", "Listening on port %d\n", port);
}
