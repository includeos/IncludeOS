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
#include <net/nat/nat.hpp>

using namespace net;
using namespace net::nat;

CASE("TCP NAT verifying rewrite and checksum")
{
  // Socket
  const Socket src{ip4::Addr{10,0,0,42},80};
  const Socket dst{ip4::Addr{10,0,0,43},32222};
  auto tcp = create_tcp_packet_init(src, dst);
  EXPECT(tcp->source() == src);
  EXPECT(tcp->destination() == dst);

  // Update checksum
  EXPECT(tcp->tcp_checksum() == 0);
  tcp->set_tcp_checksum();
  tcp->set_ip_checksum();
  EXPECT(tcp->compute_tcp_checksum() == 0);
  EXPECT(tcp->compute_ip_checksum() == 0);

  // DNAT Socket
  dnat(*tcp, src);
  EXPECT(tcp->source() == src);
  EXPECT(tcp->destination() == src);
  EXPECT(tcp->compute_tcp_checksum() == 0);
  EXPECT(tcp->compute_ip_checksum() == 0);

  // revert
  dnat(*tcp, dst);
  EXPECT(tcp->source() == src);
  EXPECT(tcp->destination() == dst);
  EXPECT(tcp->compute_tcp_checksum() == 0);
  EXPECT(tcp->compute_ip_checksum() == 0);

  // SNAT Socket
  snat(*tcp, dst);
  EXPECT(tcp->source() == dst);
  EXPECT(tcp->destination() == dst);
  EXPECT(tcp->compute_tcp_checksum() == 0);
  EXPECT(tcp->compute_ip_checksum() == 0);

  // revert
  snat(*tcp, src);
  EXPECT(tcp->source() == src);
  EXPECT(tcp->destination() == dst);
  EXPECT(tcp->compute_tcp_checksum() == 0);
  EXPECT(tcp->compute_ip_checksum() == 0);

  // Address
  const ip4::Addr dst_addr{10,10,10,10};
  const ip4::Addr src_addr{10,0,0,1};

  // DNAT Addr
  dnat(*tcp, dst_addr);
  EXPECT(tcp->ip_dst() == dst_addr);
  EXPECT(tcp->dst_port() == dst.port());
  EXPECT(tcp->source() == src);
  EXPECT(tcp->compute_tcp_checksum() == 0);
  EXPECT(tcp->compute_ip_checksum() == 0);

  // SNAT Addr
  snat(*tcp, src_addr);
  EXPECT(tcp->ip_src() == src_addr);
  EXPECT(tcp->src_port() == src.port());
  EXPECT(tcp->compute_tcp_checksum() == 0);
  EXPECT(tcp->compute_ip_checksum() == 0);

  // Port
  const uint16_t dst_port{12345};
  const uint16_t src_port{6789};

  // DNAT port
  dnat(*tcp, dst_port);
  EXPECT(tcp->ip_dst() == dst_addr);
  EXPECT(tcp->dst_port() == dst_port);
  EXPECT(tcp->compute_tcp_checksum() == 0);
  EXPECT(tcp->compute_ip_checksum() == 0);

  // SNAT port
  snat(*tcp, src_port);
  EXPECT(tcp->ip_src() == src_addr);
  EXPECT(tcp->src_port() == src_port);
  EXPECT(tcp->compute_tcp_checksum() == 0);
  EXPECT(tcp->compute_ip_checksum() == 0);
}

CASE("UDP NAT verifying rewrite")
{
  // Socket
  const Socket src{ip4::Addr{10,0,0,42},80};
  const Socket dst{ip4::Addr{10,0,0,43},32222};
  auto udp = create_udp_packet_init(src, dst);
  EXPECT(udp->source() == src);
  EXPECT(udp->destination() == dst);

  // Update checksum
  udp->set_ip_checksum();
  EXPECT(udp->compute_ip_checksum() == 0);

  // DNAT Socket
  dnat(*udp, src);
  EXPECT(udp->source() == src);
  EXPECT(udp->destination() == src);

  EXPECT(udp->compute_ip_checksum() == 0);

  // revert
  dnat(*udp, dst);
  EXPECT(udp->source() == src);
  EXPECT(udp->destination() == dst);

  EXPECT(udp->compute_ip_checksum() == 0);

  // SNAT Socket
  snat(*udp, dst);
  EXPECT(udp->source() == dst);
  EXPECT(udp->destination() == dst);

  EXPECT(udp->compute_ip_checksum() == 0);

  // revert
  snat(*udp, src);
  EXPECT(udp->source() == src);
  EXPECT(udp->destination() == dst);

  EXPECT(udp->compute_ip_checksum() == 0);

  // Address
  const ip4::Addr dst_addr{10,10,10,10};
  const ip4::Addr src_addr{10,0,0,1};

  // DNAT Addr
  dnat(*udp, dst_addr);
  EXPECT(udp->ip_dst() == dst_addr);
  EXPECT(udp->dst_port() == dst.port());
  EXPECT(udp->source() == src);

  EXPECT(udp->compute_ip_checksum() == 0);

  // SNAT Addr
  snat(*udp, src_addr);
  EXPECT(udp->ip_src() == src_addr);
  EXPECT(udp->src_port() == src.port());

  EXPECT(udp->compute_ip_checksum() == 0);

  // Port
  const uint16_t dst_port{12345};
  const uint16_t src_port{6789};

  // DNAT port
  dnat(*udp, dst_port);
  EXPECT(udp->ip_dst() == dst_addr);
  EXPECT(udp->dst_port() == dst_port);

  EXPECT(udp->compute_ip_checksum() == 0);

  // SNAT port
  snat(*udp, src_port);
  EXPECT(udp->ip_src() == src_addr);
  EXPECT(udp->src_port() == src_port);

  EXPECT(udp->compute_ip_checksum() == 0);
}

#include <net/ip4/packet_icmp4.hpp>

CASE("ICMP NAT verifying rewrite")
{
  const ip4::Addr src{10,0,0,42};
  const ip4::Addr dst{10,0,0,43};
  auto icmp = icmp4::Packet(create_ip4_packet_init(src, dst));

  auto& ip4 = icmp.ip();
  ip4.set_protocol(Protocol::ICMPv4);
  ip4.set_ip_checksum();

  EXPECT(ip4.compute_ip_checksum() == 0);

  EXPECT(ip4.ip_src() == src);
  EXPECT(ip4.ip_dst() == dst);

  // DNAT Addr
  dnat(ip4, src);
  EXPECT(ip4.ip_src() == src);
  EXPECT(ip4.ip_dst() == src);
  EXPECT(ip4.compute_ip_checksum() == 0);

  // SNAT Addr
  snat(ip4, dst);
  EXPECT(ip4.ip_src() == dst);
  EXPECT(ip4.ip_dst() == src);
  EXPECT(ip4.compute_ip_checksum() == 0);

  const Socket sock{ip4::Addr{10,10,10,10},80};
  // Socket does nothing (unsupported)
  dnat(ip4, sock);
  EXPECT(ip4.ip_src() == dst);
  EXPECT(ip4.ip_dst() == src);
  EXPECT(ip4.compute_ip_checksum() == 0);

  snat(ip4, sock);
  EXPECT(ip4.ip_src() == dst);
  EXPECT(ip4.ip_dst() == src);
  EXPECT(ip4.compute_ip_checksum() == 0);

  // Port does nothing (unsupported)
  const auto csum = ip4.ip_checksum();
  dnat(ip4, sock.port());
  EXPECT(ip4.ip_src() == dst);
  EXPECT(ip4.ip_dst() == src);
  EXPECT(ip4.ip_checksum() == csum);

  snat(ip4, sock.port());
  EXPECT(ip4.ip_src() == dst);
  EXPECT(ip4.ip_dst() == src);
  EXPECT(ip4.ip_checksum() == csum);

}

