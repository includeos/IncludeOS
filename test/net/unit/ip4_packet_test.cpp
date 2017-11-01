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
  auto ip4 = create_ip4_packet();
  ip4->init();

  ip4->set_ip_checksum();

  const auto DEFAULT_TTL = PacketIP4::DEFAULT_TTL;

  EXPECT(ip4->ip_ttl() == DEFAULT_TTL);
  EXPECT(ip4->compute_ip_checksum() == 0);

  ip4->decrement_ttl();

  EXPECT(ip4->ip_ttl() == DEFAULT_TTL-1);
  EXPECT(ip4->compute_ip_checksum() == 0);
}
