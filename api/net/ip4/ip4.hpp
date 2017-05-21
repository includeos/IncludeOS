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

#include <unordered_map>

namespace net {

  class PacketIP4;

  /** IP4 layer */
  class IP4 {
  public:

    enum class Drop_reason
    { None, Bad_source, Bad_destination, Wrong_version, Wrong_checksum,
        Unknown_proto, TTL0 };

    enum class Direction
    { Upstream, Downstream };

    using Stack = Inet<IP4>;
    using addr = ip4::Addr;
    using header = ip4::Header;
    using IP_packet = PacketIP4;
    using IP_packet_ptr = std::unique_ptr<IP_packet>;
    using downstream_arp = delegate<void(Packet_ptr, IP4::addr)>;
    using Packet_filter = delegate<IP_packet_ptr(IP_packet_ptr)>;
    using drop_handler = delegate<void(IP_packet_ptr, Direction, Drop_reason)>;
    using PMTU = uint16_t;

    /** Initialize. Sets a dummy linklayer out. */
    explicit IP4(Stack&) noexcept;

    static const addr ADDR_ANY;
    static const addr ADDR_BCAST;

    /*
      Maximum Datagram Data Size
    */
    uint16_t MDDS() const
    { return stack_.MTU() - sizeof(ip4::Header); }

    /** Upstream: Input from link layer */
    void receive(Packet_ptr);


    //
    // Delegate setters
    //

    /** Set ICMP protocol handler (upstream)*/
    void set_icmp_handler(upstream s)
    { icmp_handler_ = s; }

    /** Set UDP protocol handler (upstream)*/
    void set_udp_handler(upstream s)
    { udp_handler_ = s; }

    /** Set TCP protocol handler (upstream) */
    void set_tcp_handler(upstream s)
    { tcp_handler_ = s; }

    /** Set packet dropped handler */
    void set_drop_handler(drop_handler s)
    { drop_handler_ = s; }

    /** Set handler for packets not addressed to this interface (upstream) */
    void set_packet_forwarding(Stack::Forward_delg fwd)
    { forward_packet_ = fwd; }

    /** Set linklayer out (downstream) */
    void set_linklayer_out(downstream_arp s)
    { linklayer_out_ = s; }

    /** Assign function to determine which upstream packets gets filtered */
    void set_upstream_filter(Packet_filter f)
    { upstream_filter_ = f; }

    /** Assign function to determine which downstream packets gets filtered */
    void set_downstream_filter(Packet_filter f)
    { downstream_filter_ = f; }

    //
    // Delegate getters
    //

    upstream icmp_handler()
    { return icmp_handler_; }

    upstream udp_handler()
    { return udp_handler_; }

    upstream tcp_handler()
    { return tcp_handler_; }

    Stack::Forward_delg forward_delg()
    { return forward_packet_; }

    downstream_arp linklayer_out()
    { return linklayer_out_; }

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

    /**  Default upstream packet filter */
    IP_packet_ptr filter_upstream(IP_packet_ptr packet);

    /**  Default downstream packet filter */
    IP_packet_ptr filter_downstream(IP_packet_ptr packet);

    /**
     *  Path MTU (and Packetization Layered Path MTU Discovery) related methods
     */
    void update_path(IP4::addr dest, PMTU val);

  private:
    /** Stats */
    uint64_t& packets_rx_;
    uint64_t& packets_tx_;
    uint32_t& packets_dropped_;

    Stack& stack_;

    /**
     *  Map of Path MTUs
     *
     *  MTU (RFC4821, p. 7): Maximum Transmission Unit, the size in bytes of the largest IP packet, including the IP header and payload,
     *  that can be transmitted on a link or path
     *
     *  Link MTU (RFC4821, p. 7): Maximum Transmission Unit, i.e., maximum IP packet size in bytes, that can be conveyed in one piece
     *  over a link
     *
     *  Key: Destination address (chosen as the local representation of a path after reviewing RFC 1191, 1981 and 4821)
     *  Value: The Path MTU (the minimum link MTU of all the links in a path between a source node and a destination node)
     */
    std::unordered_map<IP4::addr, PMTU> paths_;

    /** Downstream: Linklayer output delegate */
    downstream_arp linklayer_out_ = nullptr;

    /** Upstream delegates */
    upstream icmp_handler_ = nullptr;
    upstream udp_handler_  = nullptr;
    upstream tcp_handler_  = nullptr;

    /** Packet forwarding  */
    Stack::Forward_delg forward_packet_;

    /** Packet filters */
    Packet_filter upstream_filter_;
    Packet_filter downstream_filter_;

    /** All dropped packets go here */
    drop_handler drop_handler_;

    /** Drop a packet, calling drop handler if set */
    IP_packet_ptr drop(IP_packet_ptr ptr, Direction direction, Drop_reason reason);

  }; //< class IP4

} //< namespace net

#endif
