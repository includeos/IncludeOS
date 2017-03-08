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

#ifndef NET_INET_HPP
#define NET_INET_HPP

#include <net/inet_common.hpp>
#include <hw/mac_addr.hpp>
#include <hw/nic.hpp>

namespace net {


  class TCP;
  class UDP;
  class DHClient;

  /**
   * An abstract IP-stack interface.
   * Provides a common interface for IPv4 and (future) IPv6, simplified with
   *  no constructors etc.
   **/
  template <typename IPV >
  struct Inet {
    using Stack = Inet<IPV>;
    using Forward_delg = delegate<void(Stack& source, typename IPV::IP_packet_ptr)>;
    using Route_checker = delegate<bool(typename IPV::addr)>;
    using IP_packet_factory = delegate<typename IPV::IP_packet_ptr(Protocol)>;

    template <typename IPv>
    using resolve_func = delegate<void(typename IPv::addr)>;

    virtual typename IPV::addr ip_addr() = 0;
    virtual typename IPV::addr netmask() = 0;
    virtual typename IPV::addr gateway()  = 0;
    virtual std::string        ifname() const = 0;
    virtual MAC::Addr       link_addr() = 0;
    virtual hw::Nic&           nic() = 0;

    virtual IPV&       ip_obj() = 0;
    virtual TCP&       tcp()    = 0;
    virtual UDP&       udp()    = 0;

    virtual void set_forward_delg(Forward_delg) = 0;
    virtual void set_route_checker(Route_checker) = 0;
    virtual void cache_link_ip(typename IPV::addr, MAC::Addr) = 0;
    virtual void flush_link_ip_cache() = 0;
    virtual Forward_delg forward_delg() = 0;

    virtual constexpr uint16_t MTU() const = 0;


    /** Provision empty anonymous packet **/
    virtual Packet_ptr create_packet() = 0;

    /** Delegate to provision initialized IP packet **/
    virtual IP_packet_factory ip_packet_factory() = 0;

    /** Provision empty IP packet **/
    virtual typename IPV::IP_packet_ptr create_ip_packet(Protocol) = 0;

    virtual void resolve(const std::string& hostname, resolve_func<IPV> func) = 0;

    virtual void set_gateway(typename IPV::addr server) = 0;

    virtual void set_dns_server(typename IPV::addr server) = 0;

    virtual void network_config(typename IPV::addr ip,
                                typename IPV::addr nmask,
                                typename IPV::addr gateway,
                                typename IPV::addr dnssrv = IPV::ADDR_ANY) = 0;
    virtual void reset_config() = 0;

    using dhcp_timeout_func = delegate<void(bool timed_out)>;
    virtual void negotiate_dhcp(double timeout = 10.0, dhcp_timeout_func = nullptr) = 0;

    /** Event triggered when there are available buffers in the transmit queue */
    virtual void on_transmit_queue_available(transmit_avail_delg del) = 0;

    /** Number of packets the transmit queue has room for */
    virtual size_t transmit_queue_available() = 0;

    /** Number of buffers available in the bufstore */
    virtual size_t buffers_available() = 0;

    virtual void force_start_send_queues() = 0;

    virtual void move_to_this_cpu() = 0;
    virtual int  get_cpu_id() const noexcept = 0;

  }; //< class Inet<LINKLAYER, IPV>
} //< namespace net

#endif
