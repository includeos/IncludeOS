// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017 Oslo and Akershus University College of Applied Sciences
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
#include <config>

void Service::start()
{
  using namespace net;

  auto& stacks = Interfaces::get();
  CHECKSERT(stacks.size() == 7, "There are 7 interfaces");

  INFO("Test", "Verify eth0");
  CHECKSERT(stacks[0][0] != nullptr, "eth0 is initialized");

  auto& eth0 = *stacks[0][0];
  CHECKSERT(eth0.ip_addr() == ip4::Addr(10,0,0,60), "IP address is 10.0.0.60");
  CHECKSERT(eth0.netmask() == ip4::Addr(255,255,255,0), "Netmask is 255.255.255.0");
  CHECKSERT(eth0.gateway() == ip4::Addr(10,0,0,1), "Gateway is 10.0.0.1");
  CHECKSERT(eth0.dns_addr() == eth0.gateway(), "DNS addr is same as gateway");

  INFO("Test", "Verify eth1");
  CHECKSERT(stacks[1][0] != nullptr, "eth1 is initialized");

  auto& eth1 = *stacks[1][0];
  CHECKSERT(eth1.ip_addr() == ip4::Addr(10,0,0,61), "IP address is 10.0.0.61");
  CHECKSERT(eth1.netmask() == ip4::Addr(255,255,255,0), "Netmask is 255.255.255.0");
  CHECKSERT(eth1.gateway() == ip4::Addr(10,0,0,1), "Gateway is 10.0.0.1");
  CHECKSERT(eth1.dns_addr() == ip4::Addr(8,8,8,8), "DNS addr is 8.8.8.8");

  INFO("Test", "Verify eth2");
  CHECKSERT(stacks[2][0] != nullptr, "eth2 is initialized");

  auto& eth2 = *stacks[2][0];
  static const ip4::Addr EMPTY{0};
  CHECKSERT(eth2.ip_addr() == EMPTY, "IP address is 0.0.0.0");
  CHECKSERT(eth2.netmask() == EMPTY, "Netmask is 0.0.0.0");
  CHECKSERT(eth2.gateway() == EMPTY, "Gateway is 0.0.0.0");
  CHECKSERT(eth2.dns_addr() == EMPTY, "DNS addr is 0.0.0.0");

  INFO("Test", "Verify eth3");
  CHECKSERT(stacks[3][0] != nullptr, "eth3 is initialized (but waiting for DHCP)");

  INFO("Test", "Verify eth4");
  CHECKSERT(stacks[4][0] == nullptr, "eth4 is uninitialized");

  auto& eth5 = *stacks[5][0];
  auto verify_eth5 = [](auto& eth5) {
    INFO("Test", "Verify eth5");
    CHECKSERT(eth5.ip_addr() != EMPTY, "IP not empty (%s)", eth5.ip_addr().str().c_str());
    CHECKSERT(eth5.netmask() != EMPTY, "Netmask  not empty (%s)", eth5.netmask().str().c_str());
    CHECKSERT(eth5.gateway() != EMPTY, "Gateway  not empty (%s)", eth5.gateway().str().c_str());
    CHECKSERT(eth5.dns_addr() != EMPTY, "DNS addr not empty (%s)", eth5.dns_addr().str().c_str());
    printf("SUCCESS\n");
  };
  eth5.on_config(verify_eth5);

  // Verify config array and IPv6 config works
  INFO("Test", "Verify eth6");
  CHECKSERT(stacks[6][0] != nullptr, "eth6 is initialized");

  auto& eth6 = *stacks[6][0];
  CHECKSERT(eth6.ip_addr() == ip4::Addr(10,0,0,42), "IP address is 10.0.0.42");
  CHECKSERT(eth6.netmask() == ip4::Addr(255,255,255,0), "Netmask is 255.255.255.0");
  CHECKSERT(eth6.gateway() == ip4::Addr(10,0,0,1), "Gateway is 10.0.0.1");
  CHECKSERT(eth6.dns_addr() == ip4::Addr(4,4,4,4), "DNS addr is 8.8.8.8");

  CHECKSERT(not eth6.addr6_config().empty(), "addr6 config is not empty");
  CHECKSERT(eth6.addr6_config().has(ip6::Addr{"fe80::1337"}), "addr6 config has fe80::1337");
  CHECKSERT(eth6.addr6_config().has(ip6::Addr{"fe80::42"}), "addr6 config has fe80::42");

}
