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
#include <net/inet4>
#include <net/nat/napt.hpp>

using namespace net;

void verify() {
  static int i = 0;
  if(++i == 4)
    printf("SUCCESS\n");
}

void ip_forward(Inet<IP4>& stack,  IP4::IP_packet_ptr pckt) {
  stack.ip_obj().ship(std::move(pckt));
}

void Service::start()
{
  auto& router = Inet4::ifconfig<0>(
    { 10, 0, 10, 1 }, { 255, 255, 0, 0 }, { 10, 0, 0, 1 });

  auto& laptop1 = Inet4::ifconfig<1>(
    { 10, 0, 10, 43 }, { 255, 255, 255, 0 }, router.ip_addr());

  auto& internet_host = Inet4::ifconfig<2>(
    { 10, 0, 1, 44 }, { 255, 255, 255, 0 }, router.ip_addr());

  auto& internet_client = Inet4::ifconfig<3>(
    { 10, 0, 1, 45 }, { 255, 255, 255, 0 }, router.ip_addr());

  static nat::NAPT nat;

  router.prerouting_chain().chain.push_back({nat, &nat::NAPT::nat});
  //router.postrouting_chain().chain.push_back({nat, &nat::NAPT::receive});

  router.ip_obj().set_packet_forwarding(ip_forward);

  internet_host.tcp().listen(80, [](auto conn) {
    printf("Internet page received a new connection! (%s)\n", conn->to_string().c_str());
    verify();
  });

  laptop1.tcp().connect({ internet_host.ip_addr(), 80 }, [](auto conn) {
    printf("Laptop1 connected to internet web page! (%s)\n", conn->to_string().c_str());
    verify();
  });

  nat.add_entry(8080, { laptop1.ip_addr(), 80 });

  laptop1.tcp().listen(80, [](auto conn) {
    printf("Laptop1 received a new connection! (%s)\n", conn->to_string().c_str());
    verify();
  });

  internet_client.tcp().connect({ laptop1.ip_addr(), 80 }, [](auto conn) {
    printf("Bob (internet) connected to Laptop1! (%s)\n", conn->to_string().c_str());
    verify();
  });
}
