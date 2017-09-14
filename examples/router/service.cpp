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

#include <net/super_stack.hpp>
#include <net/router.hpp>
#include <config>
#include <rapidjson/document.h>

namespace net {

  template <typename T>
  inline Route<IP4> parse_route(const T& obj)
  {
    Expects(obj.HasMember("address"));
    Expects(obj.HasMember("netmask"));
    Expects(obj.HasMember("iface"));

    ip4::Addr address{obj["address"].GetString()};
    ip4::Addr netmask{obj["netmask"].GetString()};

    ip4::Addr nexthop = (obj.HasMember("nexthop"))
      ? ip4::Addr{obj["nexthop"].GetString()} : 0;

    int N = obj["iface"].GetInt();
    auto& iface = Super_stack::get<IP4>(N);

    int cost = (not obj.HasMember("cost")) ? 100 : obj["cost"].GetInt();

    return {address, netmask, nexthop, iface, cost};
  }

  inline Router<IP4>::Routing_table
  load_routing_table_from_cfg(int N = 0)
  {
    const auto& cfg = Config::get();
    INFO("Routeconf", "Reading route table #%i...", N);

    Expects(not cfg.empty()
      && "No config found");

    rapidjson::Document doc;
    doc.Parse(cfg.data());

    Expects(doc.IsObject()
      && "Malformed config (not an object)");

    Expects(doc.HasMember("router")
      && "Router config not found");

    auto& routers = doc["router"];
    Expects(routers.IsArray()
      && "Malformed router config (not an array)");

    auto routers_arr = routers.GetArray();
    Expects(static_cast<int>(routers.Size()) > N
      && "Route table with given index do not exist");

    Expects(routers_arr[N].IsArray()
      && "Route table with given index malformed (not an array)");
    auto routes = routers_arr[N].GetArray();

    Router<IP4>::Routing_table table{};
    table.reserve(routes.Size());

    for(auto& route : routes)
      table.emplace_back(parse_route(route));

    INFO("Routeconf", "Parsed %lu routes", table.size());
    for(auto& route : table)
      INFO2("%s", route.to_string().c_str());

    return table;
  }

  static std::unique_ptr<Router<IP4>>
  create_router_from_config(Router<IP4>::Interfaces& ifaces,
                int N = 0)
  {
    return std::make_unique<Router<IP4>>(
      ifaces, load_routing_table_from_cfg(N));
  }

  template <int N = 0>
  static Router<IP4>& get_router()
  {
    static auto router = create_router_from_config(
      Super_stack::inet().ip4_stacks(), N);
    return *router;
  }

} //< namespace net

#include <service>

void Service::start()
{
  auto& router = net::get_router();

  auto& eth0 = net::Super_stack::get<net::IP4>(0);
  auto& eth1 = net::Super_stack::get<net::IP4>(1);

  eth0.set_forward_delg(router.forward_delg());
  eth1.set_forward_delg(router.forward_delg());
}
