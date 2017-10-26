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

//#define ROUTER_DEBUG 1
#ifdef ROUTER_DEBUG
#define PRINT(fmt, ...) printf("<Router> " fmt "\n", ##__VA_ARGS__)
#else
#define PRINT(fmt, ...) /* fmt */
#endif


namespace net {

  template <class IPV>
  struct Route {

    using Stack = Inet<IPV>;
    using Stack_ptr = Stack*;
    using Addr = typename IPV::addr;
    using Netmask = typename IPV::addr;

    Addr net() const noexcept
    { return net_; }

    Netmask netmask() const noexcept
    { return netmask_; }

    Addr nexthop() const noexcept
    { return nexthop_; }

     Addr nexthop(Addr ip) noexcept
    {

      if (net_ == 0)
        return nexthop_;

      if ((ip & netmask_) == net_ )
        return ip;

      return nexthop_;
    }

    int cost() const noexcept
    { return cost_; }

    Stack_ptr interface() const noexcept
    { return iface_; };

    Stack_ptr match(typename IPV::addr dest) const noexcept
    { return (dest & netmask_) == net_ ? iface_ : nullptr; }

    bool operator<(const Route& b) const
    { return cost() < b.cost(); }

    bool operator==(const Route& b) const
    {
      return net_ == b.net() and
        netmask_ == b.netmask() and
        cost_ == b.cost() and
        iface_ == b.interface();
    }

    void ship(typename IPV::IP_packet_ptr pckt, Addr nexthop) {
      iface_->ip_obj().ship(std::move(pckt), nexthop);
    }

    void ship(typename IPV::IP_packet_ptr pckt) {
      ship(std::move(pckt), nexthop(pckt->ip_dst()));
    }

    Route(Addr net, Netmask mask, Addr nexthop, Stack& iface, int cost = 100)
      : net_{net}, netmask_{mask}, nexthop_{nexthop}, iface_{&iface}, cost_{cost}
    {}

    std::string to_string() const
    {
      return net_.str() + " " + netmask_.str() + " " + nexthop_.str()
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
    using Forward_delg  = typename Inet<IPV>::Forward_delg;
    using Addr          = typename IPV::addr;
    using Packet_ptr    = typename IPV::IP_packet_ptr;
    using Interfaces    = std::vector<std::unique_ptr<Stack>>;
    using Routing_table = std::vector<Route<IPV>>;

    /**
     * Forward an IP packet according to local policy / routing table.
     **/
    inline void forward(Stack& source, typename IPV::IP_packet_ptr pckt);

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
    {  }

    void set_routing_table(Routing_table tbl) {
      routing_table_ = tbl;
    };

  private:
    Routing_table routing_table_;

  }; // < class Router

} //< namespace net

#include <net/ip4/packet_ip4.hpp>

namespace net {

  template <typename IPV>
  inline void Router<IPV>::forward(Stack&, typename IPV::IP_packet_ptr pckt)
  {
    Expects(pckt);

    if(pckt->ip_ttl() == 0)
    {
      PRINT("TTL equals 0 - dropping");
      return;
    }

    pckt->decrement_ttl();

    const auto dest = pckt->ip_dst();
    auto* route = get_most_specific_route(dest);

    if (not route) {
      PRINT("No route found for %s DROP", dest.to_string().c_str());
      return;
    }

    PRINT("Found route: %s", route->to_string().c_str());
    route->ship(std::move(pckt));
  }

} //< namespace net

#endif // NET_ROUTER_HPP
