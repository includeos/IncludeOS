// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
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
#ifndef NET_ETHERNET_HPP
#define NET_ETHERNET_HPP

#include <string>
#include "header.hpp"
#include "ethertype.hpp"
#include <hw/mac_addr.hpp> // ethernet address
#include <hw/nic.hpp> // protocol
#include <net/inet_common.hpp>

namespace net {

  /** Ethernet packet handling. */
  class Ethernet {
  public:
    static constexpr size_t MINIMUM_PAYLOAD = 46;

    // MAC address
    using addr = MAC::Addr;

    /** Constructor */
    explicit Ethernet(downstream physical_downstream, const addr& mac) noexcept;

    using header  = ethernet::Header;
    using trailer = ethernet::trailer_t;

    /** Bottom upstream input, "Bottom up". Handle raw ethernet buffer. */
    void receive(Packet_ptr);

    /** Delegate upstream IPv4 upstream. */
    void set_ip4_upstream(upstream del)
    { ip4_upstream_ = del; }

    /** Delegate upstream IPv4 upstream. */
    upstream& ip4_upstream()
    { return ip4_upstream_; }

    /** Delegate upstream IPv6 upstream. */
    void set_ip6_upstream(upstream del)
    { ip6_upstream_ = del; };

    /** Delegate upstream ARP upstream. */
    void set_arp_upstream(upstream del)
    { arp_upstream_ = del; }

    upstream& arp_upstream()
    { return arp_upstream_; }

    /** Delegate downstream */
    void set_physical_downstream(downstream del)
    { physical_downstream_ = del; }

    downstream& physical_downstream()
    { return physical_downstream_; }

    static constexpr uint16_t header_size() noexcept
    { return sizeof(ethernet::Header); }

    static constexpr hw::Nic::Proto proto() noexcept
    { return hw::Nic::Proto::ETH; }

    /** Transmit data, with preallocated space for eth.header */
    void transmit(Packet_ptr, addr dest, Ethertype);

    /** Stats getters **/
    uint64_t get_packets_rx()
    { return packets_rx_; }

    uint64_t get_packets_tx()
    { return packets_tx_; }

    uint64_t get_packets_dropped()
    { return packets_dropped_; }

  private:
    const addr& mac_;

    /** Stats */
    uint64_t& packets_rx_;
    uint64_t& packets_tx_;
    uint32_t& packets_dropped_;

    /** Upstream OUTPUT connections */
    upstream ip4_upstream_ = nullptr;
    upstream ip6_upstream_ = nullptr;
    upstream arp_upstream_ = nullptr;

    /** Downstream OUTPUT connection */
    downstream physical_downstream_ = [](Packet_ptr){};

    /*

      +--|IP4|---|ARP|---|IP6|---+
      |                          |
      |        Ethernet          |
      |                          |
      +---------|Phys|-----------+

    */
  }; //< class Ethernet
} // namespace net

#endif //< NET_ETHERNET_HPP
