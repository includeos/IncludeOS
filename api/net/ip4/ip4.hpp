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

#include <map>
#include "addr.hpp"
#include "header.hpp"
#include "packet_ip4.hpp"
#include <common>
#include <net/netfilter.hpp>
#include <net/port_util.hpp>
#include <rtc>
#include <util/timer.hpp>

#include <unordered_map>

namespace net {

  class Inet;

  /** IP4 layer */
  class IP4 {
  public:

    enum class Drop_reason
    { None, Bad_source, Bad_length, Bad_destination,
      Wrong_version, Wrong_checksum, Unknown_proto, TTL0 };

    enum class Direction
    { Upstream, Downstream };

    /**
     *  RFC 1191 Plateau table
     *  Table 7-1: Common MTUs in the Internet
     */
    enum class PMTU_plateau {
      ONE     = 68,   // RFC 791 (and RFC 1191) specifies that the official minimum MTU is 68 bytes
      TWO     = 296,
      THREE   = 508,
      FOUR    = 1006,
      FIVE    = 1492,
      SIX     = 2002
    };

    using Stack = class Inet;
    using addr = ip4::Addr;
    using header = ip4::Header;
    using IP_packet = PacketIP4;
    using IP_packet_ptr = std::unique_ptr<IP_packet>;
    using IP_packet_factory = delegate<IP_packet_ptr(Protocol)>;
    using downstream_arp = delegate<void(Packet_ptr, ip4::Addr)>;
    using drop_handler = delegate<void(IP_packet_ptr, Direction, Drop_reason)>;
    using Forward_delg  = delegate<void(IP_packet_ptr, Stack& source, Conntrack::Entry_ptr)>;
    using PMTU = uint16_t;
    using netmask = ip4::Addr;
    using resolve_func = delegate<void(ip4::Addr, const Error&)>;

    /** Initialize. Sets a dummy linklayer out. */
    explicit IP4(Stack&) noexcept;

    static const ip4::Addr ADDR_ANY;
    static const ip4::Addr ADDR_BCAST;

    /**
     * How often the pmtu_timer_ is triggered, in seconds
     */
    const int DEFAULT_PMTU_TIMER_INTERVAL = 60;

    /**
     *  Path MTU Discovery increases a path's PMTU if it hasn't been decreased in the last 10 minutes
     *  to avoid stale PMTU values
     */
    static const uint16_t DEFAULT_PMTU_AGED = 10;

    static const uint16_t PMTU_INFINITY = 0;

    /*
      Maximum Datagram Data Size
    */
    uint16_t MDDS() const;

    /**
     * @brief      Get the default MTU for the OS, including the size of the ip4::Header
     *
     * @return     The default Path MTU value
     */
    uint16_t default_PMTU() const noexcept;

    /** Upstream: Input from link layer */
    void receive(Packet_ptr, const bool link_bcast);


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

    /** Set linklayer out (downstream) */
    void set_linklayer_out(downstream_arp s)
    { linklayer_out_ = s; }

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
    void ship(Packet_ptr, ip4::Addr next_hop = 0, Conntrack::Entry_ptr ct = nullptr);


    /**
     * \brief
     *
     * Returns the IPv4 address associated with this interface
     **/
    const ip4::Addr local_ip() const;

    /**
     * @brief      Determines if the packet is for me (this host).
     *
     * @param[in]  dst   The destination
     *
     * @return     True if for me, False otherwise.
     */
    bool is_for_me(ip4::Addr dst) const;

    ///
    /// PACKET FILTERING
    ///

    /**
     * Packet filtering hooks for firewall, NAT, connection tracking etc.
     **/

    /** Packets pass through prerouting chain before routing decision */
    Filter_chain<IP4>& prerouting_chain()
    { return prerouting_chain_; }

    /** Packets pass through postrouting chain after routing decision */
    Filter_chain<IP4>& postrouting_chain()
    { return postrouting_chain_; }

    /** Packets pass through input chain before hitting protocol handlers */
    Filter_chain<IP4>& input_chain()
    { return input_chain_; }

    /** Packets pass through output chain after exiting protocol handlers */
    Filter_chain<IP4>& output_chain()
    { return output_chain_; }

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

    /**  Reassemble fragments into a coherent heap-allocated packet **/
    IP_packet_ptr reassemble(IP_packet_ptr packet);

    /**
     *  Path MTU Discovery (and Packetization Layered Path MTU Discovery) related methods
     */

    /**
     * @brief      Getter for Path MTU Discovery status
     *             RFC 1191
     *
     * @return     True if Path MTU Discovery is enabled (default)
     */
    bool path_mtu_discovery() const noexcept
    { return path_mtu_discovery_; }

    /**
     * @brief      Disable or enable Path MTU Discovery (enabled by default)
     *             RFC 1191
     *             If enabled, it sets the Don't Fragment flag on each IP4 packet
     *             TCP and UDP acts based on this being enabled or not
     *
     * @param[in]  on     Enables Path MTU Discovery if true, disables if false
     * @param[in]  aged   Number of minutes that indicate that a PMTU value
     *                    has grown stale and should be reset/increased
     *                    This could be set to "infinity" (PMTU should never be
     *                    increased) by setting the value to INFINITY
     */
    void set_path_mtu_discovery(bool on, uint16_t aged = 10) noexcept;

    /**
     * @brief      Updates the Path MTU for the specified path (represented by the destination address and port (Socket))
     *             If the path doesn't exist, a new entry/path with these input values is created
     *             This method also starts the pmtu_timer_ if it's not running
     *
     * @param[in]  dest              The destination (address and port)
     * @param[in]  new_pmtu          The new pmtu
     * @param[in]  received_too_big  Indicates that an ICMP Too Big message has been received and the method is called
     *                               as a result of this
     * @param[in]  total_length      The IP header's total length, given if the ICMP Too Big message contains a
     *                               next hop MTU value of zero. This is used to make an estimate of the new Path MTU value
     * @param[in]  header_length     The IP header's header length, given if the ICMP Too Big message contains a
     *                               next hop MTU value of zero. This is used to make an estimate of the new Path MTU value
     */
    void update_path(Socket dest, PMTU new_pmtu, bool received_too_big,
      uint16_t total_length = 0, uint8_t header_length = 0);

    /**
     * @brief      Removes a path.
     *
     * @param[in]  dest  The destination (represented by the destination Socket (IP address and port))
     */
    void remove_path(Socket dest);

    inline void flush_paths() noexcept
    { paths_.clear(); }

    /**
     * @brief      Get the Path MTU value for the specified path/destination
     *
     * @param[in]  dest  The destination (represented by the destination Socket (IP address and port))
     *
     * @return     The Path MTU value for this path/destination
     *             Returns 0 if the entry wasn't found
     */
    PMTU pmtu(Socket dest) const;

    /**
     * @brief      Get the timestamp (time since boot) for when the PMTU entry's PMTU value was last decreased
     *
     *
     * @param[in]  dest  The destination (IP address and port), used as index into paths_
     *
     * @return     The path/destination's timestamp (when it was last decreased)
     *             Returns 0 if the entry wasn't found or the PMTU for the entry has never been decreased
     */
    RTC::timestamp_t pmtu_timestamp(Socket dest) const;

    PMTU minimum_MTU() const noexcept
    { return (PMTU) PMTU_plateau::ONE; }

    ip4::Addr address() const noexcept
    { return addr_; }

    ip4::Addr networkmask() const noexcept
    { return netmask_; }

    ip4::Addr gateway() const noexcept
    { return gateway_; }

    ip4::Addr broadcast_addr() const noexcept
    { return addr_ | ( ~ netmask_); }

    void set_addr(ip4::Addr addr)
    { addr_ = addr; }

    void set_netmask(ip4::Addr netmask)
    { netmask_ = netmask; }

    void set_gateway(ip4::Addr gateway)
    { gateway_ = gateway; }

  private:
    /* Network config for the inet stack */
    ip4::Addr addr_;
    ip4::Addr netmask_;
    ip4::Addr gateway_;
    /** Stats */
    uint64_t& packets_rx_;
    uint64_t& packets_tx_;
    uint32_t& packets_dropped_;

    /**
     * Path MTU Discovery can be enabled or disabled
     * It is enabled by default and can be disabled via Inet
     *
     * @todo When Path MTU Discovery has been tested for a while, set this to true (default on)
     */
    bool path_mtu_discovery_{false};

    Stack& stack_;

    /**
     * @brief      Estimates a Path MTU value based on the Total Length field of an IP header
     *             Implemented in accordance with RFC 1191, section 5
     *
     * @param[in]  total_length  The IP header's total length value, that is used to make an
     *                           estimate of the new Path MTU value
     * @param[in]  header_length The IP header's header length value
     * @param[in]  current       The current Path MTU value for the destination
     *
     * @return     The new estimated Path MTU value
     */
    PMTU pmtu_from_total_length(uint16_t total_length, uint8_t header_length, PMTU current);

    /**
     * @brief      RFC 1191
     *             Once a minute, a timer-driven procedure runs through the routing table
     *             (here: implemented a separate PMTU cache: paths_), and for each entry
     *             whose timestamp is not "reserved" (chosen reserved value in this implementation
     *             is 0) and is older than the timeout interval:
     *             - The PMTU estimate is set to the MTU of the associated first hop
     *             - Packetization layers using this route are notified of the increase
     *
     *             RFC 1981
     *             To detect increases in a path's PMTU, a node periodically increases its
     *             assumed PMTU. This will almost always result in packets being discarded and
     *             Packet Too Big messages being generated, because in most cases the PMTU of the
     *             path will not have changed. Therefore, attempts to detect increases in a path's
     *             PMTU should be done infrequently.
     *
     *             Nodes using Path MTU Discovery MAY detect increases in PMTU, but because doing so
     *             requires sending packets larger than the current estimated PMTU, and because the likelihood
     *             is that the PMTU will not have increased, this MUST be done at infrequent intervals.
     *             An attempt to detect an increase (by sending a packet larger than the current estimate) MUST NOT
     *             be done less than 5 minutes after a Packet Too Big message has been received for the given path.
     *             The recommended setting for this timer is twice its minimum value (10 minutes).
     */
    void reset_stale_paths();

    class PMTU_entry {
    public:
      PMTU_entry(PMTU pmtu, PMTU reset_pmtu, bool received_too_big) noexcept
      : pmtu_{pmtu}, reset_pmtu_{reset_pmtu}
      {
        /* If the entry is created as a result of a received ICMP Too Big message,
        we'll set the timestamp to something other than 0.
        This way, it will be part of the aging process.

        RFC 1191: Whenever a PMTU is decreased in response to a Datagram Too Big message, the
        timestamp is set to the current time */
        if (received_too_big)
          timestamp_ = RTC::time_since_boot();
      }

      PMTU pmtu() const noexcept
      { return pmtu_; }

      /**
       * @brief      Set the PMTU for this entry/destination
       *
       *             RFC 1191: Whenever a PMTU is decreased in response to a Datagram Too Big message, the
       *             timestamp is set to the current time
       *
       * @param[in]  pmtu             The new PMTU
       * @param[in]  received_too_big The new PMTU value is set in a response to a ICMP Datagram Too Big message
       */
      void set_pmtu(PMTU pmtu, bool received_too_big) noexcept {
        pmtu_ = pmtu;

        if (received_too_big)
          timestamp_ = RTC::time_since_boot();  // timestamp set to the current "time"
      }

      RTC::timestamp_t timestamp() const noexcept
      { return timestamp_; }

      /**
       * @brief      Increases the Path MTU value on timer timeout
       *             RFC 1191 Section 6.3 and 7.1
       */
      void reset_pmtu() noexcept
      { pmtu_ = reset_pmtu_; }

    private:
      PMTU pmtu_;

      /**
       * PMTU value to reset to when timer timeouts (increase PMTU)
       * RFC 1191: An implementation should "age" cached values. When a PMTU value has not been
       * decreased for a while (on the order of 10 minutes), the PMTU estimate should be set to
       * the first-hop data-link MTU, and the packetization layers should be notified of the change.
       * This will cause the complete PMTU Discovery process to take place again.
       * Can cause dropping of datagrams every 10 minutes.
       */
      PMTU reset_pmtu_;

      RTC::timestamp_t timestamp_{0};

    };  // < class PMTU_entry

    /**
     *  Map of Path MTUs
     *
     *  MTU (RFC4821, p. 7): Maximum Transmission Unit, the size in bytes of the largest IP packet,
     *  including the IP header and payload, that can be transmitted on a link or path
     *
     *  Link MTU (RFC4821, p. 7): Maximum Transmission Unit, i.e., maximum IP packet size in bytes,
     *  that can be conveyed in one piece over a link
     *
     *  Key: Destination address (chosen as the local representation of a path after reviewing RFC 1191,
     *  1981 and 4821)
     *  Value: The Path MTU (the minimum link MTU of all the links in a path between a source node and a
     *  destination node)
     */
    std::unordered_map<Socket, PMTU_entry> paths_;

    /**
     * Timer that prevents stale PMTU values
     * When a PMTU value has not been decreased for a while (on the order of 10 minutes), the
     * PMTU estimate should be set to the first-hop data-link MTU, and the packetization layers
     * should be notified of the change
     * This will cause the complete PMTU Discovery process to take place again
     * An implementation should provide a means for changing the timeout duration, including setting
     * it to "infinity"
     * An upper layer MUST NOT retransmit datagrams in response to an increase in the PMTU estimate,
     * since this increase never comes in response to an indication of a dropped datagram
     */
    Timer pmtu_timer_{{ *this, &IP4::reset_stale_paths }};

    /**
     * How often the pmtu_timer_ should check for aged values in the PMTU cache (paths_)
     */
    std::chrono::seconds pmtu_timer_interval_{DEFAULT_PMTU_TIMER_INTERVAL};

    /**
     * How old a PMTU_entry can get before being increased/reset again (in minutes)
     * The entry's timestamp is renewed/updated every time its PMTU value is decreased in response to
     * a ICMP Datagram Too Big message
     */
    uint16_t pmtu_aged_{DEFAULT_PMTU_AGED};

    /** Downstream: Linklayer output delegate */
    downstream_arp linklayer_out_ = nullptr;

    /** Upstream delegates */
    upstream icmp_handler_ = nullptr;
    upstream udp_handler_  = nullptr;
    upstream tcp_handler_  = nullptr;

    /** Packet forwarding  */
    Forward_delg forward_packet_;

    // Filter chains
    Filter_chain<IP4> prerouting_chain_{"Prerouting", {}};
    Filter_chain<IP4> postrouting_chain_{"Postrouting", {}};
    Filter_chain<IP4> input_chain_{"Input", {}};
    Filter_chain<IP4> output_chain_{"Output", {}};
    uint32_t& prerouting_dropped_;
    uint32_t& postrouting_dropped_;
    uint32_t& input_dropped_;
    uint32_t& output_dropped_;

    /** All dropped packets go here */
    drop_handler drop_handler_;

    /** Drop a packet, calling drop handler if set */
    IP_packet_ptr drop(IP_packet_ptr ptr, Direction direction, Drop_reason reason);

  }; //< class IP4

} //< namespace net

#endif
