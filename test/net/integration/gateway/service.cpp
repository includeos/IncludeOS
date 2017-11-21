// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2016-2017 Oslo and Akershus University College of Applied Sciences
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

#include <service>
#include <net/inet4>
#include <net/nat/napt.hpp>
#include <net/router.hpp>

using namespace net;

std::unique_ptr<Router<IP4>> router;
std::shared_ptr<Conntrack> ct;
std::unique_ptr<nat::NAPT> natty;

int pongs = 0;

void verify() {
  static int i = 0;
  if (++i == 6) printf("SUCCESS\n");
}


Filter_verdict<IP4> my_dnat(IP4::IP_packet_ptr pkt, Inet<IP4>&, Conntrack::Entry_ptr entry)
{
  // Can't do anything without entry
  if(not entry)
    return {nullptr, Filter_verdict_type::DROP};

  // DNAT all TCP traffic destined to me to host2
  if(pkt->ip_protocol() == Protocol::TCP
    and pkt->ip_dst() == ip4::Addr{10, 0, 1, 1})
  {
    auto tcp = static_unique_ptr_cast<tcp::Packet>(std::move(pkt));
    // Only on dst port 5001
    if(tcp->dst_port() == 5001)
    {
      natty->dnat(*tcp, entry, {{10,0,2,10}, 5000});
      return {std::move(tcp), Filter_verdict_type::ACCEPT};
    }
  }

  return {std::move(pkt), Filter_verdict_type::ACCEPT};
}

Filter_verdict<IP4> my_fw(IP4::IP_packet_ptr pkt, Inet<IP4>&, Conntrack::Entry_ptr entry)
{
  // Can't do anything without entry
  if(not entry)
    return {nullptr, Filter_verdict_type::DROP};

  // Allow already ESTABLISHED traffic
  if(entry->state == Conntrack::State::ESTABLISHED)
    return {std::move(pkt), Filter_verdict_type::ACCEPT};

  // Pass through TCP traffic that is destined to host2
  if(pkt->ip_protocol() == Protocol::TCP
    and pkt->ip_dst() == ip4::Addr{10, 0, 2, 10})
  {
    auto tcp = static_unique_ptr_cast<tcp::Packet>(std::move(pkt));
    // Only on dst port 5000
    if(tcp->dst_port() == 5000)
    {
      return {std::move(tcp), Filter_verdict_type::ACCEPT};
    }
  }

  // Allow forwarding all ICMP traffic
  if(pkt->ip_protocol() == Protocol::ICMPv4)
  {
    return {std::move(pkt), Filter_verdict_type::ACCEPT};
  }

  return {nullptr, Filter_verdict_type::DROP};
}

Filter_verdict<IP4> deny_all(IP4::IP_packet_ptr pkt, Inet<IP4>&, Conntrack::Entry_ptr entry)
{
  // Can't do anything without entry
  if(not entry)
    return {nullptr, Filter_verdict_type::DROP};

  // Allow already ESTABLISHED traffic
  if(entry->state == Conntrack::State::ESTABLISHED)
    return {std::move(pkt), Filter_verdict_type::ACCEPT};

  // Also allow to send outgoing ping
  if(pkt->ip_protocol() == Protocol::ICMPv4)
  {
    auto icmp = icmp4::Packet(std::move(pkt));

    // Allow ping
    if(icmp.type() == icmp4::Type::ECHO)
      return {icmp.release(), Filter_verdict_type::ACCEPT};
  }

  return {nullptr, Filter_verdict_type::DROP};
}

Filter_verdict<IP4> snat_translate(IP4::IP_packet_ptr pkt, Inet<IP4>&, Conntrack::Entry_ptr entry)
{
  // Can't do anything without entry
  if(not entry)
    return {nullptr, Filter_verdict_type::DROP};
  natty->snat(*pkt, entry);
  return {std::move(pkt), Filter_verdict_type::ACCEPT};
}

void Service::start()
{
  static auto& eth0 = Inet4::ifconfig<0>(
    { 10, 0, 1, 1 }, { 255, 255, 255, 0 }, 0);

  static auto& eth1 = Inet4::ifconfig<1>(
    { 10, 0, 2, 1 }, { 255, 255, 255, 0 }, 0);

  static auto& host1 = Inet4::ifconfig<2>(
    { 10, 0, 1, 10 }, { 255, 255, 255, 0 }, eth0.ip_addr());

  static auto& host2 = Inet4::ifconfig<3>(
    { 10, 0, 2, 10 }, { 255, 255, 255, 0 }, eth1.ip_addr());

  Router<IP4>::Routing_table routing_table{
    {{10, 0, 1, 0 }, { 255, 255, 255, 0}, 0, eth0, 1 },
    {{10, 0, 2, 0 }, { 255, 255, 255, 0}, 0, eth1, 1 }
  };

  router = std::make_unique<Router<IP4>>(routing_table);
  eth0.ip_obj().set_packet_forwarding(router->forward_delg());
  eth1.ip_obj().set_packet_forwarding(router->forward_delg());

  // Setup Conntracker
  ct = std::make_shared<Conntrack>();
  eth0.enable_conntrack(ct);
  eth1.enable_conntrack(ct);
  natty = std::make_unique<nat::NAPT>(ct);

  INFO("eth0", "Prerouting: my_dnat");
  eth0.ip_obj().prerouting_chain().chain.push_back(my_dnat);
  INFO("eth0", "Postrouting: snat_translate");
  eth0.ip_obj().postrouting_chain().chain.push_back(snat_translate);

  INFO("eth0", "Prerouting: my_fw");
  eth0.ip_obj().prerouting_chain().chain.push_back(my_fw);

  INFO("eth1", "Prerouting: deny_all");
  eth1.ip_obj().prerouting_chain().chain.push_back(deny_all);

  INFO("Ping", "host1 => host2 (%s)", host2.ip_addr().to_string().c_str());
  host1.icmp().ping(host2.ip_addr(), [](auto reply) {
    CHECKSERT(reply, "Got pong from %s", host2.ip_addr().to_string().c_str());
    pongs++;
    verify();
  }, 1);

  INFO("Ping", "host1 => eth0 (%s)", eth0.ip_addr().to_string().c_str());
  host1.icmp().ping(eth0.ip_addr(), [](auto reply) {
    CHECKSERT(reply, "Got pong from %s", eth0.ip_addr().to_string().c_str());
    pongs++;
    verify();
  }, 1);

  INFO("Ping", "host1 => eth1 (%s)", eth1.ip_addr().to_string().c_str());
  host1.icmp().ping(eth1.ip_addr(), [](auto reply) {
    CHECKSERT(reply, "Got pong from %s", eth1.ip_addr().to_string().c_str());
    pongs++;
    verify();
  }, 1);

  INFO("Ping", "host2 => host1 (%s)", host1.ip_addr().to_string().c_str());
  host2.icmp().ping(host1.ip_addr(), [](auto reply) {
    CHECKSERT(reply, "Got pong from %s", host1.ip_addr().to_string().c_str());
    pongs++;
    verify();
  }, 1);

  INFO("TCP", "host2 => listen:5000");
  host2.tcp().listen(5000, [](auto) {});

  INFO("TCP", "host1 => host2 (%s:%i)", host2.ip_addr().to_string().c_str(), 5000);
  host1.tcp().connect({host2.ip_addr(), 5000}, [](auto conn) {
    CHECKSERT(conn, "Connection established with %s:%i", host2.ip_addr().to_string().c_str(), 5000);
    verify();
  });

  INFO("TCP", "host1 => eth0 (%s:%i)", eth0.ip_addr().to_string().c_str(), 5001);
  host1.tcp().connect({eth0.ip_addr(), 5001}, [](auto conn) {
    CHECKSERT(conn, "Connection established with %s:%i", eth0.ip_addr().to_string().c_str(), 5001);
    verify();
  });
}
