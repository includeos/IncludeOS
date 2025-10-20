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
#include <net/vlan>
#include <net/interfaces>

void test_finished() {
  static int i = 0;
  if (++i == 3) printf("SUCCESS\n");
}

void Service::start()
{
  auto& eth0 = net::Interfaces::get(0);
  auto& eth1 = net::Interfaces::get(1);

  net::setup_vlans();

  auto& vlan0_2 = net::Interfaces::get(0,2);
  auto& vlan0_42 = net::Interfaces::get(0,42);

  auto& vlan1_2 = net::Interfaces::get(1,2);
  auto& vlan1_42 = net::Interfaces::get(1,42);


  eth0.tcp().listen(80, [](auto conn) {
    printf("Received connection %s\n", conn->to_string().c_str());
    test_finished();
  });

  vlan0_2.tcp().listen(80, [](auto conn) {
    printf("Received connection %s\n", conn->to_string().c_str());
    test_finished();
  });

  vlan1_42.tcp().listen(80, [](auto conn) {
    printf("Received connection %s\n", conn->to_string().c_str());
    test_finished();
  });

  eth1.tcp().connect({eth0.ip_addr(), 80}, [](auto conn) {
    CHECKSERT(conn != nullptr, "eth0 can connect to eth1");
    printf("Connected to %s\n", conn->to_string().c_str());
  });

  vlan1_2.tcp().connect({vlan0_2.ip_addr(), 80}, [](auto conn) {
    CHECKSERT(conn != nullptr, "VLAN 2 can connect to peer inside the same VLAN");
    printf("Connected to %s\n", conn->to_string().c_str());
  });

  vlan0_42.tcp().connect({vlan1_42.ip_addr(), 80}, [](auto conn) {
    CHECKSERT(conn != nullptr, "VLAN 42 can connect to to peer inside the same VLAN");
    printf("Connected to %s\n", conn->to_string().c_str());
  });

}
