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

using namespace net;

CASE("IP4 Packet TTL")
{
  const ip4::Addr src{10,0,0,42};
  const ip4::Addr dst{192,168,1,200};
  auto ip4 = create_ip4_packet_init(src, dst);

  const uint8_t DEFAULT_TTL = 128;

  ip4->set_ip_ttl(DEFAULT_TTL);
  ip4->set_ip_checksum();

  EXPECT(ip4->ip_ttl() == DEFAULT_TTL);
  EXPECT(ip4->compute_ip_checksum() == 0);

  for(uint8_t i = DEFAULT_TTL; i > 0; i--) {
    ip4->decrement_ttl();
    EXPECT(ip4->ip_ttl() == i-1);
    EXPECT(ip4->compute_ip_checksum() == 0);
  }
}

CASE("IP4 Packet TTL - multiple packets")
{
  std::vector<ip4::Addr> addrs{
    {10,0,0,42}, {10,50,30,20}, {192,168,1,244}, {172,1,1,0}, {255,255,255,0},
    {10,10,10,10}, {1,1,1,1}, {0,0,0,0}, {111,222,111,222}, {123,123,0,123}
  };

  for(int i = 0; i < addrs.size()-1; i++)
  {
    auto ip4 = create_ip4_packet_init(addrs[i], addrs[i+1]);

    const uint8_t DEFAULT_TTL = 64;

    ip4->set_ip_ttl(DEFAULT_TTL);
    ip4->set_ip_checksum();

    EXPECT(ip4->ip_ttl() == DEFAULT_TTL);
    EXPECT(ip4->compute_ip_checksum() == 0);

    for(uint8_t i = DEFAULT_TTL; i > 0; i--)
      ip4->decrement_ttl();

    EXPECT(ip4->ip_ttl() == 0);
    EXPECT(ip4->compute_ip_checksum() == 0);
  }
}
