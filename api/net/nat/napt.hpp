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
#include <net/ip4/ip4.hpp>

namespace net {
namespace nat {

/**
 * @brief      Network Address Port Translator
 */
class NAPT {
public:
  using Stack = Inet;

public:

  NAPT(std::shared_ptr<Conntrack> ct);

  /**
   * @brief      Masquerade a packet, changing it's source
   *
   * @param      pkt   The packet
   * @param[in]  inet  The inet
   */
  void masquerade(IP4::IP_packet& pkt, Stack& inet, Conntrack::Entry_ptr);

  /**
   * @brief      Demasquerade a packet, restoring it's destination
   *
   * @param      pkt   The packet
   * @param[in]  inet  The inet
   */
  void demasquerade(IP4::IP_packet& pkt, const Stack& inet, Conntrack::Entry_ptr);

  void dnat(IP4::IP_packet& pkt, Conntrack::Entry_ptr, const Socket sock);
  void dnat(IP4::IP_packet& pkt, Conntrack::Entry_ptr, const ip4::Addr addr);
  void dnat(IP4::IP_packet& pkt, Conntrack::Entry_ptr, const uint16_t port);
  void dnat(IP4::IP_packet& pkt, Conntrack::Entry_ptr);

  void snat(IP4::IP_packet& pkt, Conntrack::Entry_ptr, const Socket sock);
  void snat(IP4::IP_packet& pkt, Conntrack::Entry_ptr, const ip4::Addr addr);
  void snat(IP4::IP_packet& pkt, Conntrack::Entry_ptr, const uint16_t port);
  void snat(IP4::IP_packet& pkt, Conntrack::Entry_ptr);

private:
  std::shared_ptr<Conntrack> conntrack;

  /**
   * @brief      If not already updated, bind to a ephemeral port and update the entry.
   *
   * @param[in]  entry  The entry
   * @param[in]  addr   The address
   * @param      ports  The ports
   *
   * @return     The socket used for SNAT
   */
  Socket masq(Conntrack::Entry_ptr entry, const ip4::Addr addr, Port_util& ports);

}; // < class NAPT

} // < namespace nat
} // < namespace net

#endif
