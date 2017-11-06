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

#include <common.cxx>
#include <packet_factory.hpp>
#include <net/nat/napt.hpp>
#include <nic_mock.hpp>
#include <net/inet4>

using namespace net;
using namespace net::nat;

//Nic_mock nic;
//Inet4 inet{nic};

static Conntrack::Entry* get_entry(Conntrack& ct, const PacketIP4& pkt)
{
  auto* entry = ct.in(pkt);
  ct.confirm(pkt);
  return entry;
}

static tcp::Packet_ptr tcp_packet(Socket src, Socket dst)
{
  auto tcp = create_tcp_packet_init(src, dst);
  tcp->set_tcp_checksum();
  tcp->set_ip_checksum();
  return tcp;
}

CASE("TCP NAPT DNAT")
{
  auto conntrack = std::make_shared<Conntrack>();
  NAPT napt{conntrack};

  const Socket src{{10,0,0,1}, 32222};
  const Socket dst{{10,0,0,42},80};
  const Socket target{{10,0,0,43}, 8080};

  // Request
  auto tcp = tcp_packet(src, dst);
  auto* entry = get_entry(*conntrack, *tcp);
  napt.dnat(*tcp, entry, target);

  EXPECT(tcp->source() == src);
  EXPECT(tcp->destination() == target);
  EXPECT(tcp->compute_tcp_checksum() == 0);
  EXPECT(tcp->compute_ip_checksum() == 0);

  // Reply
  tcp = tcp_packet(target, src);
  entry = get_entry(*conntrack, *tcp);
  napt.snat(*tcp, entry);

  EXPECT(tcp->source() == dst);
  EXPECT(tcp->destination() == src);
  EXPECT(tcp->compute_tcp_checksum() == 0);
  EXPECT(tcp->compute_ip_checksum() == 0);
}

CASE("TCP NAPT SNAT")
{
  auto conntrack = std::make_shared<Conntrack>();
  NAPT napt{conntrack};

  const Socket src{{10,0,0,1}, 32222};
  const Socket dst{{10,0,0,42},80};
  const ip4::Addr new_src{10,0,0,10};

  // Request
  auto tcp = tcp_packet(src, dst);
  auto* entry = get_entry(*conntrack, *tcp);
  EXPECT(entry != nullptr);
  napt.snat(*tcp, entry, new_src);

  EXPECT(tcp->source() == Socket(new_src, src.port()));
  EXPECT(tcp->destination() == dst);
  EXPECT(tcp->compute_tcp_checksum() == 0);
  EXPECT(tcp->compute_ip_checksum() == 0);

  // Reply
  tcp = tcp_packet(dst, Socket{new_src, src.port()});
  entry = get_entry(*conntrack, *tcp);
  EXPECT(entry != nullptr);
  napt.dnat(*tcp, entry);

  EXPECT(tcp->source() == dst);
  EXPECT(tcp->destination() == src);
  EXPECT(tcp->compute_tcp_checksum() == 0);
  EXPECT(tcp->compute_ip_checksum() == 0);
}
