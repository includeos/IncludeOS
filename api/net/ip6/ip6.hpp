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

#ifndef NET_IP6_IP6_HPP
#define NET_IP6_IP6_HPP

#include "addr.hpp"
#include "header.hpp"
#include "packet_ip6.hpp"
#include "addr_list.hpp"

#include <common>
#include <net/netfilter.hpp>
#include <net/conntrack.hpp>

namespace net
{

  class Inet;

  /** IP6 layer */
  class IP6
  {
  public:
    enum class Drop_reason
    { None, Bad_source, Bad_destination, Wrong_version,
        Unknown_proto };

    enum class Direction
    { Upstream, Downstream };

    using Stack = class Inet;
    using addr = ip6::Addr;
    using header = ip6::Header;
    using IP_packet = PacketIP6;
    using IP_packet_ptr = std::unique_ptr<IP_packet>;
    using IP_packet_factory = delegate<IP_packet_ptr(Protocol)>;
    using downstream_ndp = delegate<void(Packet_ptr, IP6::addr, MAC::Addr)>;
    using drop_handler = delegate<void(IP_packet_ptr, Direction, Drop_reason)>;
    using Forward_delg  = delegate<void(IP_packet_ptr, Stack& source, Conntrack::Entry_ptr)>;
    using PMTU = uint16_t;
    using netmask = uint8_t;

    static const addr ADDR_ANY;
    static const addr ADDR_LOOPBACK;

    /** Initialize. Sets a dummy linklayer out. */
    explicit IP6(Stack&) noexcept;

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
    void set_packet_forwarding(Forward_delg fwd)
    { forward_packet_ = fwd; }

    /** Set linklayer out (downstream) via ndp */
    void set_linklayer_out(downstream_ndp s)
    { ndp_out_ = s; }

    /** Upstream: Input from link layer */
    void receive(Packet_ptr, const bool link_bcast);


    //
    // Delegate getters
    //

    upstream icmp_handler()
    { return icmp_handler_; }

    upstream udp_handler()
    { return udp_handler_; }

    upstream tcp_handler()
    { return tcp_handler_; }

    Forward_delg forward_delg()
    { return forward_packet_; }

    /**
     *  Downstream: Receive data from above and transmit
     *
     *  @note: The following *must be set* in the packet:
     *
     *   * Destination IP
     *   * Protocol
     *
     *  Source IP *can* be set - if it's not, IP6 will set it
     */
    void transmit(Packet_ptr);
    void ship(Packet_ptr, addr next_hop = IP6::ADDR_ANY, Conntrack::Entry_ptr ct = nullptr);

    const ip6::Addr local_ip() const;

    ip6::Addr_list& addr_list()
    { return addr_list_; }

    const ip6::Addr_list& addr_list() const
    { return addr_list_; }

    bool is_valid_source(const ip6::Addr& addr) const
    { return addr_list_.has(addr); }

    /**
     * @brief      Determines if the packet is for me (this host).
     *
     * @param[in]  dst   The destination
     *
     * @return     True if for me, False otherwise.
     */
    bool is_for_me(ip6::Addr dst) const;
    Protocol ipv6_ext_header_receive(net::PacketIP6& packet);

    ///
    /// PACKET FILTERING
    ///

    /**
     * Packet filtering hooks for firewall, NAT, connection tracking etc.
     **/

    /** Packets pass through prerouting chain before routing decision */
    Filter_chain<IP6>& prerouting_chain()
    { return prerouting_chain_; }

    /** Packets pass through postrouting chain after routing decision */
    Filter_chain<IP6>& postrouting_chain()
    { return postrouting_chain_; }

    /** Packets pass through input chain before hitting protocol handlers */
    Filter_chain<IP6>& input_chain()
    { return input_chain_; }

    /** Packets pass through output chain after exiting protocol handlers */
    Filter_chain<IP6>& output_chain()
    { return output_chain_; }

    /*
      Maximum Datagram Data Size
    */
    uint16_t MDDS() const;

    /**
     * Stats getters
     **/
    uint64_t get_packets_rx()
    { return packets_rx_; }

    uint64_t get_packets_tx()
    { return packets_tx_; }

    uint64_t get_packets_dropped()
    { return packets_dropped_; }

    /**  Drop incoming packets invalid according to RFC */
    IP_packet_ptr drop_invalid_in(IP_packet_ptr packet);

    /**  Drop outgoing packets invalid according to RFC */
    IP_packet_ptr drop_invalid_out(IP_packet_ptr packet);

  private:
    Stack& stack_;

    ip6::Addr_list addr_list_;

    /** Stats */
    uint64_t& packets_rx_;
    uint64_t& packets_tx_;
    uint32_t& packets_dropped_;

    /** Upstream delegates */
    upstream icmp_handler_ = nullptr;
    upstream udp_handler_  = nullptr;
    upstream tcp_handler_  = nullptr;

    /** Downstream delegates */
    downstream_ndp ndp_out_ = nullptr;

    /** Packet forwarding  */
    Forward_delg forward_packet_;

    // Filter chains
    Filter_chain<IP6> prerouting_chain_{"Prerouting", {}};
    Filter_chain<IP6> postrouting_chain_{"Postrouting", {}};
    Filter_chain<IP6> input_chain_{"Input", {}};
    Filter_chain<IP6> output_chain_{"Output", {}};

    /** All dropped packets go here */
    drop_handler drop_handler_;

    /** Drop a packet, calling drop handler if set */
    IP_packet_ptr drop(IP_packet_ptr ptr, Direction direction, Drop_reason reason);

  }; //< class IP6

} //< namespace net

#endif
