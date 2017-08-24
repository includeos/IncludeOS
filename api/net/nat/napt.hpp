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
#ifndef NET_NAT_NAPT_HPP
#define NET_NAT_NAPT_HPP

#include <map>
#include <net/port_util.hpp>
#include <net/conntrack.hpp>
#include <net/inet>
#include <net/ip4/ip4.hpp>

namespace net {
namespace nat {

/**
 * @brief      Network Address Port Translator
 */
class NAPT {
public:
  using Stack = Inet<IP4>;

public:

  NAPT(Conntrack* ct);

  /**
   * @brief      Masquerade a packet
   *
   * @param      pkt   The packet
   * @param[in]  inet  The inet
   */
  void masquerade(IP4::IP_packet& pkt, const Stack& inet);

  /**
   * @brief      Demasquerade a packet
   *
   * @param      pkt   The packet
   * @param[in]  inet  The inet
   */
  void demasquerade(IP4::IP_packet& pkt, const Stack& inet);

private:
  Port_util tcp_ports;
  Port_util udp_ports;

  Conntrack* conntrack;

  void tcp_masq(IP4::IP_packet& pkt, const Stack& inet);

  void tcp_demasq(IP4::IP_packet& pkt, const Stack& inet);

  void udp_masq(IP4::IP_packet& pkt, const Stack& inet);

  void udp_demasq(IP4::IP_packet& pkt, const Stack& inet);

}; // < class NAPT

} // < namespace nat
} // < namespace net

#endif
