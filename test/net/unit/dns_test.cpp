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
  auto& inet_client = net::Interfaces::get(1);
  inet_client.network_config({10,0,0,43}, {255,255,255,0}, {10,0,0,42});
}

CASE("Setup network")
{
  Timers::init(
    [] (Timers::duration_t) {},
    [] () {}
  );
  setup_inet();
}

#include <net/dns/dns.hpp>
CASE("DNS::question_string returns string representation of DNS record type")
{
  EXPECT(net::DNS::question_string(DNS_TYPE_A) == "IPv4 address");
}

CASE("Failing DNS request")
{
  auto& inet = net::Interfaces::get(1);

  static bool done = false;
  inet.resolve("www.google.com",
    [] (net::IP4::addr, const net::Error&) {
      done = true;
    });
  for (int i = 0; i < 10; i++)
  {
    //printf("BEF Done = %d\n", done);
    Events::get().process_events();
    //printf("AFT Done = %d\n", done);
  }
}
