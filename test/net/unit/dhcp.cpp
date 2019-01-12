// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2017 Oslo and Akershus University College of Applied Sciences
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

#include <common.cxx>
#include <net/inet>
#include <net/interfaces>
#include <hw/async_device.hpp>

static std::unique_ptr<hw::Async_device<UserNet>> dev1 = nullptr;
static std::unique_ptr<hw::Async_device<UserNet>> dev2 = nullptr;

static void setup_inet()
{
  dev1 = std::make_unique<hw::Async_device<UserNet>>(UserNet::create(1500));
  dev2 = std::make_unique<hw::Async_device<UserNet>>(UserNet::create(1500));
  dev1->connect(*dev2);
  dev2->connect(*dev1);

  auto& inet_server = net::Interfaces::get(0);
  inet_server.network_config({10,0,0,42}, {255,255,255,0}, {10,0,0,43});
}

CASE("Setup network")
{
  Timers::init(
    [] (Timers::duration_t) {},
    [] () {}
  );
  setup_inet();
}

#include <net/dhcp/dhcpd.hpp>
static std::unique_ptr<net::dhcp::DHCPD> dhcp_server = nullptr;
CASE("Setup DHCP server")
{
  auto& inet = net::Interfaces::get(0);
  dhcp_server = std::make_unique<net::dhcp::DHCPD> (
      inet.udp(), net::ip4::Addr{10,0,0,1}, net::ip4::Addr{10,0,0,24});
  dhcp_server->listen();
}

CASE("Create DHCP request")
{
  auto& inet = net::Interfaces::get(1);
  static bool done = false;
  inet.on_config(
    [] (net::Inet& inet) {
      //assert(inet.ip_addr() == net::ip4::Addr{10,0,0,1});
      printf("Configured!\n");
      done = true;
    });
  
  inet.negotiate_dhcp();

  while (!done)
  {
    //printf("BEF Done = %d\n", done);
    Events::get().process_events();
    //printf("AFT Done = %d\n", done);
  }
}
