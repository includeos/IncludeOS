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

#include <map>
#include <net/inet.hpp>

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

    Addr gateway() const noexcept
    { return gateway_; }

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

    Route(Addr net, Netmask mask, Addr gateway, Stack& iface, int cost)
      : net_{net}, netmask_{mask}, gateway_{gateway}, iface_{&iface}, cost_{cost}
    {}

  private:
    Addr net_;
    Netmask netmask_;
    Addr gateway_;
    Stack_ptr iface_;
    int cost_;
  };


  template<class IPV>
  struct Router {

    using Stack   = typename Route<IPV>::Stack;
    using Stack_ptr = typename Route<IPV>::Stack_ptr;
    using Forward_delg = typename Inet<IPV>::Forward_delg;
    using Addr    = typename IPV::addr;
    using Interfaces = std::vector<std::unique_ptr<Stack>>;
    using Routing_table = std::vector<Route<IPV>>;

    /**
     * Forward an IP packet according to local policy / routing table.
     **/
    void forward(Stack& source, typename IPV::IP_packet_ptr);

    /**
     * Get forwarding delegate
     * (And ensure forward signature Matches the signature of IP forwarding delegate.)
     **/
    Forward_delg forward_delg()
    { return Forward_delg(*this, forward); }


    /** Get any interface route for a certain IP **/
    Route<IPV>* get_first_route(typename IPV::addr dest) {

      for (auto&& route : routing_table_) {
        Stack_ptr match = route.match(dest);
        if (match) return &route;
      }

      return nullptr;
    };

    /** Get any interface route for a certain IP **/
    Stack_ptr get_first_interface(typename IPV::addr dest) {
      auto route = get_first_route(dest);
      if (route) return route->interface();
      return nullptr;
    };

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


    /** Construct a router over a set of interfaces **/
    Router(Interfaces& ifaces, Routing_table tbl = {})
      : networks_{ifaces}, routing_table_{tbl}
    {  }

    void set_routing_table(Routing_table tbl) {
      routing_table_ = tbl;
    };

  private:
    Interfaces& networks_;
    Routing_table routing_table_;

  };

} //< namespace net

#endif // NET_ROUTER_HPP
