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

#pragma once
#ifndef NET_NAT_NAT_HPP
#define NET_NAT_NAT_HPP

#include <net/socket.hpp>
#include <net/ip4/packet_ip4.hpp>

namespace net {
namespace nat {

/* TCP Source NAT */
void tcp_snat(PacketIP4& pkt, const Socket& new_sock);
void tcp_snat(PacketIP4& pkt, const ip4::Addr new_addr);
void tcp_snat(PacketIP4& pkt, const uint16_t new_port);
/* UDP Source NAT */
void udp_snat(PacketIP4& pkt, const Socket& new_sock);
void udp_snat(PacketIP4& pkt, const ip4::Addr new_addr);
void udp_snat(PacketIP4& pkt, const uint16_t new_port);
/* ICMP Source NAT */
void icmp_snat(PacketIP4& pkt, const ip4::Addr new_addr);
/* IP4 Source NAT (depending on specified protocol) */
void snat(PacketIP4& pkt, const Socket& src_socket);
void snat(PacketIP4& pkt, const ip4::Addr new_addr);
void snat(PacketIP4& pkt, const uint16_t new_port);

/* TCP Destination NAT */
void tcp_dnat(PacketIP4& pkt, const Socket& new_sock);
void tcp_dnat(PacketIP4& pkt, const ip4::Addr new_addr);
void tcp_dnat(PacketIP4& pkt, const uint16_t new_port);
/* UDP Destination NAT */
void udp_dnat(PacketIP4& pkt, const Socket& new_sock);
void udp_dnat(PacketIP4& pkt, const ip4::Addr new_addr);
void udp_dnat(PacketIP4& pkt, const uint16_t new_port);
/* ICMP Destination NAT */
void icmp_dnat(PacketIP4& pkt, const ip4::Addr new_addr);
/* IP4 Destination NAT (depending on specified protocol) */
void dnat(PacketIP4& pkt, const Socket& dst_socket);
void dnat(PacketIP4& pkt, const ip4::Addr new_addr);
void dnat(PacketIP4& pkt, const uint16_t new_port);

}
}

#endif
