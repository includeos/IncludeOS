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
#include <net/ip4/ip4.hpp>

namespace net {
namespace nat {

/**
 * @brief      Source NAT
 *
 * @param      pkt         The packet
 * @param[in]  src_socket  The source socket
 */
void snat(IP4::IP_packet& pkt, Socket src_socket);

void tcp_snat(IP4::IP_packet& pkt, Socket new_sock);

void udp_snat(IP4::IP_packet& pkt, Socket new_sock);

void icmp_snat(IP4::IP_packet& pkt, const ip4::Addr new_addr);

/**
 * @brief      Destination NAT
 *
 * @param      pkt         The packet
 * @param[in]  dst_socket  The destination socket
 */
void dnat(IP4::IP_packet& pkt, Socket dst_socket);
void dnat(IP4::IP_packet& pkt, const ip4::Addr new_addr);
void dnat(IP4::IP_packet& pkt, const uint16_t new_port);

void tcp_dnat(IP4::IP_packet& pkt, Socket new_sock);
void tcp_dnat(IP4::IP_packet& pkt, const ip4::Addr new_addr);
void tcp_dnat(IP4::IP_packet& pkt, const uint16_t new_port);

void udp_dnat(IP4::IP_packet& pkt, Socket new_sock);
void udp_dnat(IP4::IP_packet& pkt, const ip4::Addr new_addr);
void udp_dnat(IP4::IP_packet& pkt, const uint16_t new_port);

void icmp_dnat(IP4::IP_packet& pkt, const ip4::Addr new_addr);

}
}

#endif
