// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2016-2017 Oslo and Akershus University College of Applied Sciences
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
#include <net/interfaces>
#include <net/dhcp/dhcpd.hpp>
#include <list>

std::unique_ptr<net::dhcp::DHCPD> server;

void Service::start(const std::string&)
{
  using namespace net;
  using namespace dhcp;

  // Server

  auto& inet = Interfaces::get(0);
  inet.network_config(
    { 10,200,0,1 },     // IP
    { 255,255,0,0 },    // Netmask
    { 10,0,0,1 },       // Gateway
    { 8,8,8,8 });       // DNS

  IP4::addr pool_start{10,200,100,20};
  IP4::addr pool_end{10,200,100,30};
  server = std::make_unique<DHCPD>(inet.udp(), pool_start, pool_end);

  INFO("<Service>", "Service started");
}
