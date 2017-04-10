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

#include <common.cxx>
#include <net/router.hpp>

#include <net/ip4/ip4.hpp>
#include <net/inet.hpp>

using namespace net;

// We need discrete inet pointers for the tests
Inet<IP4>* eth1 = (Inet<IP4>*) 1;
Inet<IP4>* eth2 = (Inet<IP4>*) 2;
Inet<IP4>* eth3 = (Inet<IP4>*) 3;
Inet<IP4>* eth4 = (Inet<IP4>*) 4;


CASE("Creating and matching against routes")
{

  Route<IP4> r1{{10, 42, 42, 0 }, { 255, 255, 255, 0}, {10, 42, 42, 2}, *eth1 , 1 };

  EXPECT(r1.netmask() == (IP4::addr{255, 255, 255, 0}));
  EXPECT(r1.net() == (IP4::addr{10, 42, 42, 0}));
  EXPECT(r1.gateway() == (IP4::addr{10, 42, 42, 2}));
  EXPECT(r1.cost() == 1);
  EXPECT(r1.interface() == eth1);


  EXPECT(r1.match({10, 42, 42, 3}));
  EXPECT(not r1.match({10, 42, 43, 3}));
  EXPECT(not r1.match({255, 255, 255, 255}));

  // Routes are copy-constructible etc.
  Route<IP4> r2 = r1;
  EXPECT(r2.interface() == r1.interface());
  EXPECT(r2 == r1);

}


CASE("Creating and using a router and routing table")
{

  Router<IP4>::Routing_table tbl{
    {{10, 42, 42, 0 }, { 255, 255, 255, 0}, {10, 42, 42, 2}, *eth1 , 2 },
    {{10, 42,  0, 0 }, { 255, 255,   0, 0}, {10, 42, 42, 3}, *eth2 , 2 },
    {{50, 42,  0, 0 }, { 255, 255,   0, 0}, {50, 42, 42, 2}, *eth3 , 3 },
    {{160, 60, 0, 0 }, { 255, 255,   0, 0}, {50, 42, 42, 2}, *eth4 , 4 },
    {{160, 60, 0, 0 }, { 255, 255,   0, 0}, {50, 42, 42, 2}, *eth1 , 4 },
    {{160, 60, 0, 0 }, { 255, 255,   0, 0}, {50, 42, 42, 2}, *eth2 , 4 }
  };


  // Routing tables can be sorted, filtered, (transformed) etc.
  Router<IP4>::Routing_table sorted = tbl;
  std::sort(sorted.begin(), sorted.end());

  Router<IP4>::Routing_table filtered{};
  std::copy_if(tbl.begin(), tbl.end(), std::back_inserter(filtered), [](const Route<IP4>& r){ return r.match({10,42,42,3}) != nullptr; });


  EXPECT(sorted.size() == tbl.size());
  EXPECT(filtered.size() == 2u);

  // Provide a (empty) set of interfaces
  std::vector<std::unique_ptr<Inet<net::IP4>>> ifaces{};

  // Construct router object
  Router<IP4> router(ifaces, tbl);

  // Check for route to a couple of IP's
  EXPECT(router.route_check({10,42,42,5}));
  EXPECT(not router.route_check({100,43,43,5}));

  // Get all routes for an IP
  auto routes = router.get_all_routes({10,42,42,10});
  EXPECT(routes.size() == 2u);
  EXPECT((routes.front().gateway() == IP4::addr{10,42,42,2} or routes.front().gateway() == IP4::addr{10,42,42,3}));

  // Get the cheapest route for an IP
  auto route = router.get_cheapest_route({10,42,42,10});
  EXPECT((route->gateway() == IP4::addr{10,42,42,2} and route->interface() == eth1));

  // Change the routing table
  router.set_routing_table( {{{10, 42, 43, 0 }, { 255, 255, 255, 0}, {10, 42, 42, 2}, *eth1 , 2 }} );

  // Now there's no cheapest route
  EXPECT(not router.get_cheapest_route({10,42,42,10}));



}
