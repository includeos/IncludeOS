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

#include <service>
#include <net/inet4>
#include <mana/server.hpp>

using namespace mana;
using namespace std::string_literals;

std::unique_ptr<Server> server;

void Service::start(const std::string&)
{
  // Setup stack; try DHCP
  auto& stack = net::Inet4::ifconfig(3.0);
  // Static config
  stack.network_config({ 10,0,0,42 },     // IP
                       { 255,255,255,0 }, // Netmask
                       { 10,0,0,1 },      // Gateway
                       { 8,8,8,8 });      // DNS

  // Create a router
	Router router;
  // Setup a route on GET /
  router.on_get("/", [](auto, auto res) {
    res->add_body("<html><body><h1>Simple example</1></body></html>"s);
    res->send();
  });

  // Create and setup the server
  server = std::make_unique<Server>(stack);
  server->set_routes(router).listen(80);
}
