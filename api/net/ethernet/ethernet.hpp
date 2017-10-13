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
    explicit Ethernet(
          downstream physical_downstream,
          const addr& mac) noexcept;

    using header  = ethernet::Header;
    using trailer = ethernet::trailer_t;

    // The link-layer decices the devices name due to construction order
    std::string link_name() const {
      return "eth" + std::to_string(ethernet_idx);
    }

    /** Bottom upstream input, "Bottom up". Handle raw ethernet buffer. */
    void receive(Packet_ptr);


    /** Protocol handler getters */
    upstream_ip& ip4_upstream()
    { return ip4_upstream_; }

    upstream_ip& ip6_upstream()
    { return ip6_upstream_; }

    upstream& arp_upstream()
    { return arp_upstream_; }


    /** Delegate upstream IPv4 upstream. */
    void set_ip4_upstream(upstream_ip del)
    { ip4_upstream_ = del; }

    /** Delegate upstream IPv6 upstream. */
    void set_ip6_upstream(upstream_ip del)
    { ip6_upstream_ = del; };

    /** Delegate upstream ARP upstream. */
    void set_arp_upstream(upstream del)
    { arp_upstream_ = del; }


    /**
     * @brief      Sets the vlan upstream.
     *
     * @param[in]  del   The upstream delegate
     */
    void set_vlan_upstream(upstream del)
    { vlan_upstream_ = del; }

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

    uint32_t get_trailer_packets_dropped()
    { return trailer_packets_dropped_; }

  protected:
    const addr& mac_;
    int   ethernet_idx;

    /** Stats */
    uint64_t& packets_rx_;
    uint64_t& packets_tx_;
    uint32_t& packets_dropped_;
    uint32_t& trailer_packets_dropped_;

    /** Upstream OUTPUT connections */
    upstream_ip ip4_upstream_ = nullptr;
    upstream_ip ip6_upstream_ = nullptr;
    upstream arp_upstream_ = nullptr;
    upstream vlan_upstream_ = nullptr;

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
