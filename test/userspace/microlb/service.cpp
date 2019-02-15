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
#include <microLB>
#include <net/interfaces>

// the configuration
static void setup_networks()
{
  extern void create_network_device(int N, const char* ip);
  create_network_device(0, "10.0.0.1/24");

  auto& inet_client = net::Interfaces::get(0);
  inet_client.network_config({10,0,0,42}, {255,255,255,0}, {10,0,0,1});

  //auto& inet_server = net::Interfaces::get(1);
  //inet_server.network_config({10,0,1,40}, {255,255,255,0}, {10,0,1,1});
}

void Service::start()
{
  setup_networks();

  // the load balancer
  static auto* balancer = new microLB::Balancer(true);

  // open for TCP connections on client interface
  auto& inet_client = net::Interfaces::get(0);
  balancer->open_for_s2n(inet_client, 443, "/test.pem", "/test.key");

  auto& inet_server = net::Interfaces::get(0);
  // add regular TCP nodes
  for (uint16_t i = 6001; i <= 6004; i++) {
    const net::Socket socket{{10,0,0,1}, i};
    balancer->nodes.add_node(socket,
        microLB::Balancer::connect_with_tcp(inet_server, socket));
  }

  balancer->nodes.on_session_close =
    [] (int idx, int current, int total) {
      printf("LB session closed %d (%d current, %d total)\n", idx, current, total);
      if (total >= 5) os::shutdown();
    };
}
