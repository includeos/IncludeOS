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
Inet* eth1 = (Inet*) 1;
Inet* eth2 = (Inet*) 2;
Inet* eth3 = (Inet*) 3;
Inet* eth4 = (Inet*) 4;

CASE("net::router: Testing Route functionality")
{
  const Route<IP4> r1{{10, 42, 42, 0 }, { 255, 255, 255, 0}, {10, 42, 42, 2}, *eth1 , 2 };
  const Route<IP4> r2{{10, 42, 0, 0 }, { 255, 255, 0, 0}, {10, 42, 42, 3}, *eth2 , 1 };
  const Route<IP4> r3{{0}, {0}, {10, 0, 0, 1}, *eth3 , 1 };

  // Matches
  EXPECT(r1.match({10,42,42,1}) == eth1);
  EXPECT(r1.match({10,42,43,1}) == nullptr);

  EXPECT(r2.match({10,42,43,1}) == eth2);
  EXPECT(r2.match({10,43,0,1}) == nullptr);

  // Eats it all
  EXPECT(r3.match({10,42,42,1}) != nullptr);
  EXPECT(r3.match({10,42,43,1}) != nullptr);
  EXPECT(r3.match({192,168,1,100}) != nullptr);

  // Operators
  EXPECT(not (r1 == r2));
  EXPECT(r2 < r1);
}

CASE("net::router: Creating and matching against routes")
{

  Route<IP4> r1{{10, 42, 42, 0 }, { 255, 255, 255, 0}, {10, 42, 42, 2}, *eth1 , 1 };

  EXPECT(r1.netmask() == (IP4::addr{255, 255, 255, 0}));
  EXPECT(r1.net() == (IP4::addr{10, 42, 42, 0}));
  EXPECT(r1.nexthop() == (IP4::addr{10, 42, 42, 2}));
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


CASE("net::router: Creating and using a router and routing table")
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

  // Construct router object
  Router<IP4> router(tbl);

  // Check for route to a couple of IP's
  EXPECT(router.route_check({10,42,42,5}));
  EXPECT(not router.route_check({100,43,43,5}));

  // Get all routes for an IP
  auto routes = router.get_all_routes({10,42,42,10});
  EXPECT(routes.size() == 2u);
  EXPECT((routes.front().nexthop() == IP4::addr{10,42,42,2} or routes.front().nexthop() == IP4::addr{10,42,42,3}));

  // Get the cheapest route for an IP
  auto route = router.get_cheapest_route({10,42,42,10});
  EXPECT((route->nexthop() == IP4::addr{10,42,42,2} and route->interface() == eth1));

  // Getting the most specific route should hit the most specific one (duh)
  route = router.get_most_specific_route({10,42,42,10});

  EXPECT(route != nullptr);
  EXPECT(route->nexthop() == IP4::addr(10,42,42,2));

  // Change the routing table
  router.set_routing_table( {{{10, 42, 43, 0 }, { 255, 255, 255, 0}, {10, 42, 42, 2}, *eth1 , 2 }} );

  // Now there's no cheapest route
  EXPECT(not router.get_cheapest_route({10,42,42,10}));

}

CASE("net::router: Testing default gateway route")
{
  const ip4::Addr gateway{10,0,0,1};
  Router<IP4>::Routing_table tbl{
    {{10, 42, 42, 0 }, { 255, 255, 255, 0}, {10, 42, 42, 2}, *eth1 , 2 },
    {{10, 42,  0, 0 }, { 255, 255,   0, 0}, {10, 42, 42, 3}, *eth2 , 2 },
    {{0}, {0}, gateway, *eth1}
  };


  Router<IP4> router(tbl);
  auto route = router.get_most_specific_route({10,42,42,10});
  EXPECT(route->nexthop() == IP4::addr(10,42,42,2));

  route = router.get_most_specific_route({10,42,43,10});
  EXPECT(route->nexthop() == IP4::addr(10,42,42,3));

  route = router.get_most_specific_route({10,43,42,10});
  EXPECT(route->nexthop() == IP4::addr(10,0,0,1));

}

#include <nic_mock.hpp>
#include <packet_factory.hpp>
#include <net/inet>

CASE("net::router: Actual routing verifying TTL")
{
  Nic_mock nic1;
  Inet inet1{nic1};
  inet1.network_config({10,0,1,1},{255,255,255,0}, 0);

  Nic_mock nic2;
  Inet inet2{nic2};
  inet2.network_config({10,0,2,1},{255,255,255,0}, 0);

  Router<IP4>::Routing_table tbl{
    {{10, 0, 1, 0}, {255, 255, 255, 0}, {0}, inet1 , 1 },
    {{10, 0, 2, 0}, {255, 255, 255, 0}, {0}, inet2 , 1 }
  };

  Router<IP4> router(tbl);

  inet1.set_forward_delg(router.forward_delg());
  inet2.set_forward_delg(router.forward_delg());

  const Socket src{ip4::Addr{10,0,1,10}, 32222};
  const Socket dst{ip4::Addr{10,0,2,10}, 80};
  const uint8_t DEFAULT_TTL = PacketIP4::DEFAULT_TTL;

  // Here we gonna receive the ICMP TTL Exceeded ONCE
  static int time_exceeded_count = 0;
  inet1.ip_obj().set_linklayer_out([&](auto pckt, auto ip) {
    auto packet = static_unique_ptr_cast<net::PacketIP4>(std::move(pckt));
    EXPECT(packet->ip_protocol() == Protocol::ICMPv4);
    EXPECT(packet->ip_ttl() == PacketIP4::DEFAULT_TTL);

    auto icmp = icmp4::Packet(std::move(packet));
    ICMP_error err{icmp.type(), icmp.code()};
    EXPECT(err.icmp_type() == icmp4::Type::TIME_EXCEEDED);
    EXPECT(err.icmp_code() == static_cast<uint8_t>(icmp4::code::Time_exceeded::TTL));

    time_exceeded_count++;
  });

  // Here we gonna receive a TCP Packet ONCE
  static int tcp_packet_recv = 0;
  inet2.ip_obj().set_linklayer_out([&](auto pckt, auto ip) {
    auto packet = static_unique_ptr_cast<tcp::Packet>(std::move(pckt));
    EXPECT(packet->source() == src);
    EXPECT(packet->destination() == dst);
    EXPECT(packet->ip_ttl() == (PacketIP4::DEFAULT_TTL-1));

    tcp_packet_recv++;
  });

  // Here we gonna receive a ICMP TTL Exceeded
  auto tcp = create_tcp_packet_init(src, dst);
  tcp->set_ip_ttl(0);
  tcp->set_ip_checksum();
  tcp->set_tcp_checksum();
  inet1.ip_obj().receive(std::move(tcp), false);
  EXPECT(time_exceeded_count == 1);
  EXPECT(tcp_packet_recv == 0);

  // without send_time_exceeded
  router.send_time_exceeded = false;
  tcp = create_tcp_packet_init(src, dst);
  tcp->set_ip_ttl(0);
  tcp->set_ip_checksum();
  tcp->set_tcp_checksum();
  // this one simply gets dropped (not another ICMP)
  inet1.ip_obj().receive(std::move(tcp), false);
  EXPECT(time_exceeded_count == 1);
  EXPECT(tcp_packet_recv == 0);

  // Ok this one is actual legit (default TTL)
  router.send_time_exceeded = true;
  tcp = create_tcp_packet_init(src, dst);
  tcp->set_ip_ttl(PacketIP4::DEFAULT_TTL);
  tcp->set_ip_checksum();
  tcp->set_tcp_checksum();
  inet1.ip_obj().receive(std::move(tcp), false);
  EXPECT(time_exceeded_count == 1);
  EXPECT(tcp_packet_recv == 1);

  // Test the forward chain as well
  // Accept the first one
  router.forward_chain.chain.push_back([](
    IP4::IP_packet_ptr pkt, Inet&, Conntrack::Entry_ptr)->Filter_verdict<IP4>
  {
    return Filter_verdict<IP4>{std::move(pkt), Filter_verdict_type::ACCEPT};
  });

  tcp = create_tcp_packet_init(src, dst);
  tcp->set_ip_checksum();
  tcp->set_tcp_checksum();
  inet1.ip_obj().receive(std::move(tcp), false);
  EXPECT(tcp_packet_recv == 2);

  // Lets drop the next one
  router.forward_chain.chain.push_back([](
    IP4::IP_packet_ptr pkt, Inet&, Conntrack::Entry_ptr)->Filter_verdict<IP4>
  {
    return Filter_verdict<IP4>{std::move(pkt), Filter_verdict_type::DROP};
  });

  tcp = create_tcp_packet_init(src, dst);
  tcp->set_ip_checksum();
  tcp->set_tcp_checksum();
  inet1.ip_obj().receive(std::move(tcp), false);
  EXPECT(tcp_packet_recv == 2);
}

CASE("net::router: Calculate Route nexthop")
{
  Nic_mock nic1;
  Inet inet1{nic1};
  inet1.network_config({10, 0, 2, 1}, {255, 255, 255, 0}, 0);

  // Net for inet1 without nexthop
  const Route<IP4> r1{{10, 0, 2, 0}, {255, 255, 0, 0}, 0, inet1};
  // Matches route and interfaces net, can be sent directly
  EXPECT(r1.nexthop({10,0,2,20}) == ip4::Addr(10,0,2,20)); // == ip
  // Matches routes net, but not the interface's net, need to go via nexthop
  EXPECT(r1.nexthop({10,0,1,20}) == 0); // == nexthop
  // Outside routes net (and it doesnt match the route...), need to go via nexthop
  EXPECT(r1.nexthop({10,1,0,20}) == 0); // == nexthop

  // Alias net for inet1 with nexthop to nearby router
  const Route<IP4> r2{{10, 0, 6, 0}, {255, 255, 255, 0}, {10, 0, 2, 10}, inet1};
  // Matches routes net, but not the interface's net, need to go via nexthop
  EXPECT(r2.nexthop({10,0,6,20}) == ip4::Addr(10,0,2,10)); // == nexthop
  // Ok, this is really weird because you will never ask for nexthop on this route
  // (but interface will match)..
  EXPECT(r2.nexthop({10,0,2,20}) == ip4::Addr(10,0,2,20)); // == ip

  Nic_mock nic2;
  Inet inet2{nic2};
  inet2.network_config({10, 0, 1, 1}, {255, 255, 255, 0}, 0);

  // Default route
  const Route<IP4> r3{{0,0,0,0}, {0,0,0,0}, {10, 0, 1, 1}, inet2};
  // Matches routes net (duh), but not the interface, go via nexthop
  EXPECT(r3.nexthop({10,0,2,10})  == ip4::Addr(10,0,1,1)); // == nexthop
  // Matches routes net (duh), and can be sent directly
  EXPECT(r3.nexthop({10,0,1,10})  == ip4::Addr(10,0,1,10)); // == ip
}
