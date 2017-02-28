// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
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

#ifndef NET_IP4_IP4_HPP
#define NET_IP4_IP4_HPP

#include "addr.hpp"
#include "header.hpp"
#include <common>
#include <net/inet.hpp>

namespace net {

  class PacketIP4;

  /** IP4 layer */
  class IP4 {
  public:
    using Stack = Inet<IP4>;
    using addr = ip4::Addr;
    using header = ip4::Header;
    using IP_packet = PacketIP4;
    using IP_packet_ptr = std::unique_ptr<IP_packet>;
    using downstream_arp = delegate<void(Packet_ptr, IP4::addr)>;

    /** Initialize. Sets a dummy linklayer out. */
    explicit IP4(Stack&) noexcept;

    static const addr ADDR_ANY;
    static const addr ADDR_BCAST;

    /*
      Maximum Datagram Data Size
    */
    inline constexpr uint16_t MDDS() const
    { return stack_.MTU() - sizeof(ip4::Header); }

    /** Upstream: Input from link layer */
    void receive(Packet_ptr);

    /** Upstream: Outputs to transport layer */
    inline void set_icmp_handler(upstream s)
    { icmp_handler_ = s; }

    inline void set_udp_handler(upstream s)
    { udp_handler_ = s; }

    inline void set_tcp_handler(upstream s)
    { tcp_handler_ = s; }

    /** Downstream: Delegate linklayer out */
    void set_linklayer_out(downstream_arp s)
    { linklayer_out_ = s; };

    void set_packet_forwarding(Stack::Forward_delg fwd)
    { forward_packet_ = fwd; }

    /**
     *  Downstream: Receive data from above and transmit
     *
     *  @note: The following *must be set* in the packet:
     *
     *   * Destination IP
     *   * Protocol
     *
     *  Source IP *can* be set - if it's not, IP4 will set it
     */
    void transmit(Packet_ptr);
    void ship(Packet_ptr);


    /**
     * \brief
     *
     * Returns the IPv4 address associated with this interface
     **/
    const addr local_ip() const {
      return stack_.ip_addr();
    }

    /**
     * Stats getters
     **/
    uint64_t get_packets_rx()
    { return packets_rx_; }

    uint64_t get_packets_tx()
    { return packets_tx_; }

    uint64_t get_packets_dropped()
    { return packets_dropped_; }

  private:
    /** Stats */
    uint64_t& packets_rx_;
    uint64_t& packets_tx_;
    uint32_t& packets_dropped_;

    Stack& stack_;

    /** Downstream: Linklayer output delegate */
    downstream_arp linklayer_out_ = nullptr;

    /** Upstream delegates */
    upstream icmp_handler_ = nullptr;
    upstream udp_handler_  = nullptr;
    upstream tcp_handler_  = nullptr;

    /** Packet forwarding  */
    Stack::Forward_delg forward_packet_;

  }; //< class IP4


} //< namespace net

#endif
