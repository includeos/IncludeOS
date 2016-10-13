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
#include <hw/nic.hpp>
#include <hw/mac_addr.hpp>
#include <net/inet_common.hpp>

namespace net {

  /** Ethernet packet handling. */
  class Ethernet {
  public:
    static constexpr size_t MINIMUM_PAYLOAD = 46;

    /**
     *  Some big-endian ethernet types
     *
     *  From http://en.wikipedia.org/wiki/EtherType
     */
    enum ethertype_le {
      _ETH_IP4   = 0x0800,
      _ETH_ARP   = 0x0806,
      _ETH_WOL   = 0x0842,
      _ETH_IP6   = 0x86DD,
      _ETH_FLOW  = 0x8808,
      _ETH_JUMBO = 0x8870
    };

    /** Little-endian ethertypes. */
    enum ethertype {
      ETH_IP4   = 0x8,
      ETH_ARP   = 0x608,
      ETH_WOL   = 0x4208,
      ETH_IP6   = 0xdd86,
      ETH_FLOW  = 0x888,
      ETH_JUMBO = 0x7088,
      ETH_VLAN  = 0x81
    };

    // MAC address
    using addr = hw::MAC_addr;

    static const addr MULTICAST_FRAME;
    static const addr BROADCAST_FRAME;

    static const addr IPv6mcast_01;
    static const addr IPv6mcast_02;

    /** Constructor */
    explicit Ethernet(hw::Nic& nic) noexcept;

    using header  = ethernet::Header;
    using trailer = ethernet::trailer_t;

    /** Bottom upstream input, "Bottom up". Handle raw ethernet buffer. */
    void bottom(Packet_ptr);

    /** Delegate upstream ARP handler. */
    void set_arp_handler(upstream del)
    { arp_handler_ = del; }

    upstream get_arp_handler()
    { return arp_handler_; }

    /** Delegate upstream IPv4 handler. */
    void set_ip4_handler(upstream del)
    { ip4_handler_ = del; }

    /** Delegate upstream IPv4 handler. */
    upstream get_ip4_handler()
    { return ip4_handler_; }

    /** Delegate upstream IPv6 handler. */
    void set_ip6_handler(upstream del)
    { ip6_handler_ = del; };

    /** Delegate downstream */
    void set_physical_out(downstream del)
    { physical_out_ = del; }

    /** @return Mac address of the underlying device */
    const auto& mac() const noexcept
    { return nic_.mac(); }

    /** Transmit data, with preallocated space for eth.header */
    void transmit(Packet_ptr);

  private:
    hw::Nic& nic_;

    /** Stats */
    uint64_t& packets_rx_;
    uint64_t& packets_tx_;
    uint32_t& packets_dropped_;

    /** Upstream OUTPUT connections */
    upstream ip4_handler_ = [](Packet_ptr){};
    upstream ip6_handler_ = [](Packet_ptr){};
    upstream arp_handler_ = [](Packet_ptr){};

    /** Downstream OUTPUT connection */
    downstream physical_out_ = [](Packet_ptr){};

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
