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

    Addr dest_net()
    { return dest_net_; }

    Netmask netmask()
    { return netmask_; }

    Addr gateway()
    { return gateway_; }

    int cost()
    { return cost_; }

    Stack_ptr interface()
    { return iface_; };

    Route(Addr dest_net, Netmask mask, Addr gateway, Stack_ptr iface, int cost)
      : dest_net_{dest_net}, netmask_{mask}, gateway_{gateway}, iface_{iface}, cost_{cost}
    {}

  private:
    Addr dest_net_;
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
    using Interfaces = std::vector<Stack_ptr>;
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


    /** Get the interface route for a certain IP **/
    virtual Stack_ptr get_interface(typename IPV::addr dest) {

      for (auto&& route : routing_table_) {
        if ((dest & route.netmask()) == route.dest_net())
          return route.interface();
      }

      return nullptr;

    };

    /** Construct a router over a set of interfaces **/
    Router(Interfaces&& ifaces, Routing_table&& tbl = {})
      : networks_{std::move(ifaces)}, routing_table_{tbl}
    {}

    void set_routing_table(Routing_table&& tbl) {
      routing_table_ = std::forward(tbl);
    };

  private:
    Interfaces networks_;
    Routing_table routing_table_;

  };

} //< namespace net

#endif // NET_ROUTER_HPP
