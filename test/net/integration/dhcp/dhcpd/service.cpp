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

#include <service>
#include <net/inet4>
#include <net/dhcp/dhcpd.hpp>

#include <timers>

std::unique_ptr<net::dhcp::DHCPD> server;

void Service::start(const std::string&)
{
  using namespace net;
  using namespace dhcp;

  // Server

  auto& inet = Inet4::ifconfig<0>(
    { 10,0,100,1 },     // IP
    { 255,255,0,0 },    // Netmask
    { 10,0,0,1 },       // Gateway
    { 10,0,0,1 });      // DNS

  IP4::addr pool_start{10,0,100,13};
  IP4::addr pool_end{10,0,100,20};
  server = std::make_unique<DHCPD>(inet.udp(), pool_start, pool_end);

  // Client 1

  auto& inet1 = Inet4::ifconfig<1>(10.0, [&] (bool timeout) {
    if (timeout)
      printf("Interface 1 timed out\n");
    else
      printf("Interface 1 succeed\n");
  });

  // Client 2

  auto& inet2 = Inet4::ifconfig<2>(10.0, [&] (bool timeout) {
    if (timeout)
      printf("Interface 2 timed out\n");
    else
      printf("Interface 2 succeed\n");
  });

  // Client 3

  auto& inet3 = Inet4::ifconfig<3>(10.0, [&] (bool timeout) {
    if (timeout)
      printf("Interface 3 timed out\n");
    else
      printf("Interface 3 succeed\n");
  });
}
