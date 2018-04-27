// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2017 Oslo and Akershus University College of Applied Sciences
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

#ifndef NET_INET_HPP
#define NET_INET_HPP

#include <chrono>
#include <unordered_set>
#include <net/inet_common.hpp>
#include <hw/mac_addr.hpp>
#include <hw/nic.hpp>
#include <map>
#include <net/port_util.hpp>
#include "conntrack.hpp"

namespace net {

  class   TCP;
  class   UDP;
  class   DHClient;
  class   ICMPv4;

  /**
   * An abstract IP-stack interface.
   * Provides a common interface for IPv4 and (future) IPv6, simplified with
   *  no constructors etc.
   **/
  template <typename IPV>
  struct Inet {
    using Stack         = Inet<IPV>;
    using IP_packet_ptr = typename IPV::IP_packet_ptr;
    using IP_addr       = typename IPV::addr;

    using Forward_delg  = delegate<void(IP_packet_ptr, Stack& source, Conntrack::Entry_ptr)>;
    using Route_checker = delegate<bool(IP_addr)>;
    using IP_packet_factory = delegate<IP_packet_ptr(Protocol)>;

    template <typename IPv>
    using resolve_func = delegate<void(typename IPv::addr, const Error&)>;

    using Vip_list    = std::vector<IP_addr>;
    using Port_utils  = std::map<IP_addr, Port_util>;


    ///
    /// NETWORK CONFIGURATION
    ///

    /** Get IP address of this interface **/
    virtual IP_addr ip_addr() const = 0;

    /** Get netmask of this interface **/
    virtual IP_addr netmask() const = 0;

    /** Get default gateway for this interface **/
    virtual IP_addr gateway() const = 0;

    /** Get default dns for this interface **/
    virtual IP_addr dns_addr() const = 0;

    /** Get broadcast address for this interface **/
    virtual IP_addr broadcast_addr() const = 0;

   /** Set default gateway for this interface */
    virtual void set_gateway(IP_addr server) = 0;

    /** Set DNS server for this interface */
    virtual void set_dns_server(IP_addr server) = 0;

    /** Configure network for this interface */
    virtual void network_config(IP_addr ip,
                                IP_addr nmask,
                                IP_addr gateway,
                                IP_addr dnssrv = IPV::ADDR_ANY) = 0;

    /** Reset network configuration for this interface */
    virtual void reset_config() = 0;

    using dhcp_timeout_func = delegate<void(bool timed_out)>;

    /** Use DHCP to configure this interface */
    virtual void negotiate_dhcp(double timeout = 10.0, dhcp_timeout_func = nullptr) = 0;

    virtual bool is_configured() const = 0;

    using on_configured_func = delegate<void(Stack&)>;

    /** Assign callback to when the stack has been configured */
    virtual void on_config(on_configured_func handler) = 0;

    /** Get a list of virtual IP4 addresses assigned to this interface */
    virtual const Vip_list virtual_ips() const = 0;

    /** Check if an IP is a (possibly virtual) loopback address */
    virtual bool is_loopback(IP_addr a) const = 0;

    /** Add an IP address as a virtual loopback IP */
    virtual void add_vip(IP_addr a) = 0;

    /** Remove an IP address from the virtual loopback IP list */
    virtual void remove_vip(IP_addr a) = 0;

    /** Determine the appropriate source address for a destination. */
    virtual IP_addr get_source_addr(IP_addr dest) = 0;

    /** Determine if an IP address is a valid source address for this stack */
    virtual bool is_valid_source(IP_addr) const = 0;


    /**
     * @brief      Get the active conntrack for this stack.
     *
     * @return     The conntrack for this stack, nullptr if none.
     */
    virtual std::shared_ptr<Conntrack>& conntrack() = 0;
    virtual const std::shared_ptr<Conntrack>& conntrack() const = 0;

    /**
     * @brief      Enables the conntrack for this stack,
     *             setting up the necessary hooks.
     *
     * @param      <unnamed>  The conntrack to assign to this stack.
     */
    virtual void enable_conntrack(std::shared_ptr<Conntrack>) = 0;


    ///
    /// PROTOCOL OBJECTS
    ///

    /** Get the IP protocol object for this interface */
    virtual IPV& ip_obj() = 0;

    /** Get the TCP protocol object for this interface */
    virtual TCP& tcp() = 0;

    /** Get the UDP protocol object for this interface */
    virtual UDP& udp() = 0;

    /** Get the ICMP protocol object for this interface */
    virtual ICMPv4& icmp() = 0;

    virtual Port_utils& tcp_ports() = 0;
    virtual Port_utils& udp_ports() = 0;

    ///
    /// PATH MTU DISCOVERY
    /// PACKETIZATION LAYER PATH MTU DISCOVERY
    /// ERROR REPORTING
    /// Communication from ICMP and IP to UDP and TCP
    ///

    /**
     * @brief      Disable or enable Path MTU Discovery (enabled by default)
     *             RFC 1191
     *             If enabled, it sets the Don't Fragment flag on each IP4 packet
     *             TCP and UDP acts based on this being enabled or not
     *
     * @param[in]  on    Enables Path MTU Discovery if true, disables if false
     * @param[in]  aged  Number of minutes that indicate that a PMTU value
     *                   has grown stale and should be reset/increased
     *                   This could be set to "infinity" (PMTU should never be
     *                   increased) by setting the value to IP4::INFINITY
     */
    virtual void set_path_mtu_discovery(bool on, uint16_t aged = 10) = 0;

    /**
     * @brief      Triggered by IP when a Path MTU value has grown stale and the value
     *             is reset (increased) to check if the PMTU for the path could have increased
     *             This is NOT a change in the Path MTU in response to receiving an ICMP Too Big message
     *             and no retransmission of packets should take place
     *
     * @param[in]  dest  The destination/path
     * @param[in]  pmtu  The reset PMTU value
     */
    virtual void reset_pmtu(Socket dest, typename IPV::PMTU pmtu) = 0;

    /**
     *  Error reporting
     *
     *  Including ICMP error report in accordance with RFC 1122 and handling of ICMP
     *  too big messages in accordance with RFC 1191, 1981 and 4821 (Path MTU Discovery
     *  and Packetization Layer Path MTU Discovery)
     *
     *  Forwards errors to the transport layer (UDP and TCP)
    */
    virtual void error_report(Error& err, Packet_ptr orig_pckt) = 0;

    ///
    /// DNS
    ///

    /** DNS resolution */
    virtual void resolve(const std::string& hostname,
                         resolve_func<IPV>  func,
                         bool               force = false) = 0;
    virtual void resolve(const std::string& hostname,
                         IP_addr            server,
                         resolve_func<IPV>  func,
                         bool               force = false) = 0;

    virtual void set_domain_name(std::string domain_name) = 0;

    virtual const std::string& domain_name() const = 0;

    ///
    /// LINK LAYER
    ///

    /** Get the network interface device */
    virtual hw::Nic& nic() = 0;

    /** Get interface name for this interface **/
    virtual std::string ifname() const = 0;

    /** Get linklayer address for this interface **/
    virtual MAC::Addr link_addr() const = 0;

    /** Add cache entry to the link / IP address cache */
    virtual void cache_link_addr(IP_addr, MAC::Addr) = 0;

    /** Flush  link / IP address cache */
    virtual void flush_link_cache() = 0;

    /** Set the regular interval for link address cache flushing */
    virtual void set_link_cache_flush_interval(std::chrono::minutes) = 0;


    ///
    /// ROUTING
    ///

    /** Set an IP forwarding delegate. E.g. used to enable routing.
     *  NOTE: The packet forwarder is expected to call the forward_chain
     **/
    virtual void set_forward_delg(Forward_delg) = 0;

    /** Assign boolean function to determine if we have route to a given IP */
    virtual void set_route_checker(Route_checker) = 0;

    /** Get the IP forwarding delegate */
    virtual Forward_delg forward_delg() = 0;


    ///
    /// PACKET MANAGEMENT
    ///

    /** Get Maximum Transmission Unit **/
    virtual uint16_t MTU() const = 0;

    /** Provision empty anonymous packet **/
    virtual Packet_ptr create_packet() = 0;

    /** Delegate to provision initialized IP packet **/
    virtual IP_packet_factory ip_packet_factory() = 0;

    /** Provision empty IP packet **/
    virtual IP_packet_ptr create_ip_packet(Protocol) = 0;

    /** Event triggered when there are available buffers in the transmit queue */
    virtual void on_transmit_queue_available(transmit_avail_delg del) = 0;

    /** Number of packets the transmit queue has room for */
    virtual size_t transmit_queue_available() = 0;

    /** Number of buffers available in the bufstore */
    virtual size_t buffers_available() = 0;

    /** Number of total buffers in the bufstore */
    virtual size_t buffers_total() = 0;

    /** Start TCP (e.g. after system suspension). */
    virtual void force_start_send_queues() = 0;


    ///
    /// SMP
    ///

    /** Move this interface to the CPU executing the call */
    virtual void move_to_this_cpu() = 0;
    virtual int  get_cpu_id() const noexcept = 0;


    /** Empty virtual destructor for Inet base **/
    virtual ~Inet<IPV>() {}

  }; //< class Inet<LINKLAYER, IPV>
} //< namespace net

#endif
