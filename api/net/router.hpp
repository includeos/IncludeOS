// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2016 Oslo and Akershus University College of Applied Sciences
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

#ifndef NET_ROUTER_HPP
#define NET_ROUTER_HPP

#include <net/inet.hpp>
#include <net/netfilter.hpp>

//#define ROUTER_DEBUG 1
#ifdef ROUTER_DEBUG
#define PRINT(fmt, ...) printf("<Router> " fmt "\n", ##__VA_ARGS__)
#else
#define PRINT(fmt, ...) /* fmt */
#endif


namespace net {

  template <class IPV>
  struct Route {

    using Stack = Inet;
    using Stack_ptr = Stack*;
    using Addr = typename IPV::addr;
    using Netmask = typename IPV::netmask;
    using Packet_ptr = typename IPV::IP_packet_ptr;

    Addr net() const noexcept
    { return net_; }

    Netmask netmask() const noexcept
    { return netmask_; }

    Addr nexthop() const noexcept
    { return nexthop_; }

    Addr nexthop(Addr ip)const noexcept;

    int cost() const noexcept
    { return cost_; }

    Stack_ptr interface() const noexcept
    { return iface_; };

    Stack_ptr match(typename IPV::addr dest) const noexcept
    { return (dest & netmask_) == net_ ? iface_ : nullptr; }

    bool operator<(const Route& b) const noexcept
    { return cost() < b.cost(); }

    bool operator==(const Route& b) const noexcept
    {
      return net_ == b.net() and
        netmask_ == b.netmask() and
        cost_ == b.cost() and
        iface_ == b.interface();
    }

    void ship(Packet_ptr pckt, Addr nexthop, Conntrack::Entry_ptr ct);

    void ship(typename IPV::IP_packet_ptr pckt, Conntrack::Entry_ptr ct) {
      auto next = nexthop(pckt->ip_dst());
      ship(std::move(pckt), next, ct);
    }

    Route(Addr net, Netmask mask, Addr nexthop, Stack& iface, int cost = 100)
      : net_{net}, netmask_{mask}, nexthop_{nexthop}, iface_{&iface}, cost_{cost}
    {
      Expects(iface_ != nullptr);
    }

    std::string to_string() const
    {
      return net_.str() + " " + " " + nexthop_.str()
      + " " + iface_->ifname() + " " + std::to_string(cost_);
    }

  private:
    Addr net_;
    Netmask netmask_;
    Addr nexthop_;
    Stack_ptr iface_;
    int cost_;
  };


  template<class IPV>
  struct Router {

    using Stack         = typename Route<IPV>::Stack;
    using Stack_ptr     = typename Route<IPV>::Stack_ptr;
    using Forward_delg  = typename Inet::Forward_delg;
    using Addr          = typename IPV::addr;
    using Packet_ptr    = typename IPV::IP_packet_ptr;
    using Interfaces    = std::vector<std::unique_ptr<Stack>>;
    using Routing_table = std::vector<Route<IPV>>;

    /**
     * Forward an IP packet according to local policy / routing table.
     **/
    inline void forward(Packet_ptr pckt, Stack& stack, Conntrack::Entry_ptr ct);

    /**
     * Get forwarding delegate
     * (And ensure forward signature Matches the signature of IP forwarding delegate.)
     **/
    Forward_delg forward_delg()
    { return {this, &Router<IPV>::forward}; }


    /** Get any interface route for a certain IP **/
    Route<IPV>* get_first_route(Addr dest) {

      for (auto&& route : routing_table_) {
        Stack_ptr match = route.match(dest);
        if (match) return &route;
      }

      return nullptr;
    }

    /** Get any interface route for a certain IP **/
    Stack_ptr get_first_interface(Addr dest) {
      auto route = get_first_route(dest);
      if (route) return route->interface();
      return nullptr;
    }

    /** Check if there exists a route for a given IP **/
    bool route_check(typename IPV::addr dest){
      return get_first_interface(dest) != nullptr;
    }


    /**
     * Get all routes for a certain IP
     * @todo : Optimize!
     **/
    Routing_table get_all_routes(typename IPV::addr dest) {

      Routing_table t;
      std::copy_if(routing_table_.begin(),
                   routing_table_.end(),
                   std::back_inserter(t), [dest](const Route<IPV>& route) {
                     return route.match(dest);
                   });
      return t;
    }

    /**
     * Get cheapest route for a certain IP
     * @todo : Optimize!
     **/
    Route<IPV>* get_cheapest_route(typename IPV::addr dest) {
      Routing_table all = get_all_routes(dest);
      std::sort(all.begin(), all.end());
      if (not all.empty()) return &all.front();
      return nullptr;
    };



    /**
     * Get most specific route for a certain IP
     * (e.g. the route with the largest netmask)
     * @todo : Optimize!
     **/
    Route<IPV>* get_most_specific_route(typename IPV::addr dest)
    {
      Route<IPV>* match = nullptr;
      for (auto& route : routing_table_)
        {
          if (route.match(dest)) {
            if (match) {
              match = route.netmask() > match->netmask() ? &route : match;
            } else {
              match = &route;
            }
          }
        }
      return match;
    }


    /** Construct a router over a set of interfaces **/
    Router(Routing_table tbl = {})
      : routing_table_{tbl}
    {
      INFO("Router", "Router created with %lu routes", tbl.size());
      for(auto& route : routing_table_)
        INFO2("%s", route.to_string().c_str());
    }

    void set_routing_table(Routing_table tbl) {
      routing_table_ = tbl;
    };

    /** Whether to send ICMP Time Exceeded when TTL is zero */
    bool send_time_exceeded = true;

    /** Packets pass through forward chain before being forwarded */
    Filter_chain<IPV> forward_chain{"Forward", {}};

  private:
    Routing_table routing_table_;

  }; // < class Router

} //< namespace net

#include <net/ip4/packet_ip4.hpp>
#include <net/ip4/icmp4.hpp>
#include <net/ip6/packet_ip6.hpp>
#include <net/ip6/icmp6.hpp>

namespace net {

  template <>
  inline void Route<IP4>::ship(Packet_ptr pckt, Addr nexthop, Conntrack::Entry_ptr ct) {
    iface_->ip_obj().ship(std::move(pckt), nexthop, ct);
  }

  template <>
  inline void Route<IP6>::ship(Packet_ptr pckt, Addr nexthop, Conntrack::Entry_ptr ct) {
    iface_->ip6_obj().ship(std::move(pckt), nexthop, ct);
  }

  template<>
  inline IP4::addr Route<IP4>::nexthop(IP4::addr ip) const noexcept
  {
      // No need to go via nexthop if IP is on the same net as interface
      if ((ip & iface_->netmask()) == (iface_->ip_addr() & iface_->netmask()))
        return ip;

      return nexthop_;
  }

  template<>
  inline IP6::addr Route<IP6>::nexthop(IP6::addr ip) const noexcept
  {
      // No need to go via nexthop if IP is on the same net as interface
      if ((ip & iface_->netmask6()) == (iface_->ip6_addr() & iface_->netmask6()))
        return ip;

      return nexthop_;
  }

  template <>
  inline void Router<IP4>::forward(Packet_ptr pckt, Stack& stack, Conntrack::Entry_ptr ct)
  {
    Expects(pckt);

    // Do not forward packets when TTL is 0
    if(pckt->ip_ttl() == 0)
    {
      PRINT("TTL equals 0 - dropping");
      // Send ICMP Time Exceeded if on. RFC 1812, page 84
      if(this->send_time_exceeded == true and not pckt->ip_dst().is_multicast())
        stack.icmp().time_exceeded(std::move(pckt), icmp4::code::Time_exceeded::TTL);
      return;
    }

    // When the packet originates from our host, it still passes forward
    // We add this check to prevent decrementing TTL for "none routed" packets. Hmm.
    const bool should_decrement_ttl = not stack.is_valid_source(pckt->ip_src());
    if(should_decrement_ttl)
      pckt->decrement_ttl();


    // Call the forward chain
    auto res = forward_chain(std::move(pckt), stack, ct);
    if (res == Filter_verdict_type::DROP) return;

    Ensures(res.packet != nullptr);
    pckt = res.release();

    // Look for a route
    const auto dest = pckt->ip_dst();
    auto* route = get_most_specific_route(dest);

    if(route) {
      PRINT("Found route: %s", route->to_string().c_str());
      route->ship(std::move(pckt), ct);
      return;
    }
    else {
      PRINT("No route found for %s DROP", dest.to_string().c_str());
      return;
    }
  }

  template <>
  inline void Router<IP6>::forward(Packet_ptr pckt, Stack& stack, Conntrack::Entry_ptr ct)
  {
    Expects(pckt);

    // Do not forward packets when TTL is 0
    if(pckt->hop_limit() == 0)
    {
      PRINT("TTL equals 0 - dropping");
      if(this->send_time_exceeded == true and not pckt->ip_dst().is_multicast())
        stack.icmp6().time_exceeded(std::move(pckt), icmp6::code::Time_exceeded::HOP_LIMIT);
      return;
    }

    // When the packet originates from our host, it still passes forward
    // We add this check to prevent decrementing TTL for "none routed" packets. Hmm.
    const bool should_decrement_hop_limit = not stack.is_valid_source(pckt->ip_src());
    if(should_decrement_hop_limit)
      pckt->decrement_hop_limit();


    // Call the forward chain
    auto res = forward_chain(std::move(pckt), stack, ct);
    if (res == Filter_verdict_type::DROP) return;

    Ensures(res.packet != nullptr);
    pckt = res.release();

    // Look for a route
    const auto dest = pckt->ip_dst();
    auto* route = get_most_specific_route(dest);

    if(route) {
      PRINT("Found route: %s", route->to_string().c_str());
      route->ship(std::move(pckt), ct);
      return;
    }
    else {
      PRINT("No route found for %s DROP", dest.to_string().c_str());
      return;
    }
  }

} //< namespace net

#endif // NET_ROUTER_HPP
