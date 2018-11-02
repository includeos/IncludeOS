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

#include <packet_factory.hpp>
#include <common.cxx>
#include <net/ip6/addr.hpp>

using namespace net;

CASE("IP6 Packet HOPLIMIT")
{
  const ip6::Addr src{0xfe80, 0, 0, 0, 0xe823, 0xfcff, 0xfef4, 0x85bd};
  const ip6::Addr dst{ 0xfe80,  0,  0, 0, 0xe823, 0xfcff, 0xfef4, 0x83e7 };
  auto ip6 = create_ip6_packet_init(src, dst);

  const uint8_t DEFAULT_HOPLIMIT = 128;

  ip6->set_ip_hop_limit(DEFAULT_HOPLIMIT);

  EXPECT(ip6->hop_limit() == DEFAULT_HOPLIMIT);

  for(uint8_t i = DEFAULT_HOPLIMIT; i > 0; i--) {
    ip6->decrement_hop_limit();
    EXPECT(ip6->hop_limit() == i-1);
  }
}

CASE("IP6 Packet HOPLIMIT - multiple packets")
{
  std::vector<ip6::Addr> addrs{
    {0xfe80, 0, 0, 0, 0xe823, 0xfcff, 0xfef4, 0x85bd},
    {0xfe80,  0,  0, 0, 0xe823, 0xfcff, 0xfef4, 0x83e7},
    ip6::Addr{0,0,0,1},
    {0xfe80, 0, 0, 0, 0x0202, 0xb3ff, 0xff1e, 0x8329},
  };

  for(int i = 0; i < addrs.size()-1; i++)
  {
    auto ip6 = create_ip6_packet_init(addrs[i], addrs[i+1]);

    const uint8_t DEFAULT_HOPLIMIT = 64;

    ip6->set_ip_hop_limit(DEFAULT_HOPLIMIT);

    EXPECT(ip6->hop_limit() == DEFAULT_HOPLIMIT);

    for(uint8_t i = DEFAULT_HOPLIMIT; i > 0; i--)
      ip6->decrement_hop_limit();

    EXPECT(ip6->hop_limit() == 0);
  }
}
