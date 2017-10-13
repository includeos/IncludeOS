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
#ifndef NET_ETHERNET_8021Q_HPP
#define NET_ETHERNET_8021Q_HPP

#include "ethernet.hpp"

namespace net {

namespace ethernet {
/**
 * @brief      IEEE 802.1Q header
 *
 *             Currently no support for Double tagging.
 */
struct VLAN_header {
  MAC::Addr dest;
  MAC::Addr src;
  uint16_t  tpid;
  uint16_t  tci;
  Ethertype type;

  int vid() const
  { return ntohs(this->tci) & 0xFFF; }

  void set_vid(int id)
  { tci = htons(id & 0xFFF); }

}__attribute__((packed));

} // < namespace ethernet

/**
 * @brief      Ethernet module with support for VLAN.
 *
 * @note       Inheritance out of lazyness.
 */
class Ethernet_8021Q : public Ethernet {
public:
  using header  = ethernet::VLAN_header;

  /**
   * @brief      Constructs a Ethernet_8021Q object,
   *             same as Ethernet with addition to the VLAN id
   *
   * @param[in]  phys_down  The physical down
   * @param[in]  mac        The mac addr
   * @param[in]  id         The VLAN id
   */
  explicit Ethernet_8021Q(downstream phys_down, const addr& mac, const int id) noexcept;

  void receive(Packet_ptr pkt);

  void transmit(Packet_ptr pkt, addr dest, Ethertype type);

  static constexpr uint16_t header_size() noexcept
  { return sizeof(ethernet::VLAN_header); }

private:
  const int id_;
};

} // < namespace net

#endif
