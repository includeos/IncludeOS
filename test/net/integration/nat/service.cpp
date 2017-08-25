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
std::unique_ptr<Conntrack> ct;
std::unique_ptr<nat::NAPT> natty;

void ip_forward(Inet<IP4>& stack,  IP4::IP_packet_ptr pckt) {
  // Packet could have been erroneously moved prior to this call
  if (not pckt)
    return;

  Inet<IP4>* route = router->get_first_interface(pckt->ip_dst());

  if (not route){
    INFO("ip_fwd", "No route found for %s dropping", pckt->ip_dst().to_string().c_str());
    return;
  }

  if (route == &stack) {
    INFO("ip_fwd", "* Oh, this packet was for me, sow why was it forwarded here?");
    return;
  }

  //printf("ip_fwd transmitting packet to %s\n", route->ifname().c_str());
  route->ip_obj().ship(std::move(pckt));
}

void Service::start()
{
  auto& eth0 = Inet4::ifconfig<0>(
    { 10, 1, 0, 1 }, { 255, 255, 255, 0 }, { 10, 0, 0, 1 });

  auto& eth1 = Inet4::ifconfig<1>(
    { 192, 1, 0, 1 }, { 255, 255, 255, 0 }, { 10, 0, 0, 1 });

  auto& laptop1 = Inet4::ifconfig<2>(
    { 10, 1, 0, 10 }, { 255, 255, 255, 0 }, eth0.ip_addr());

  auto& internet_host = Inet4::ifconfig<3>(
    { 192, 1, 0, 192 }, { 255, 255, 255, 0 }, eth1.ip_addr());

  Router<IP4>::Routing_table routing_table{
    {{10, 1, 0, 0 }, { 255, 255, 255, 0}, {10, 1, 0, 1}, eth0 , 1 },
    {{192, 1, 0, 0 }, { 255, 255, 255, 0}, {192, 1, 0, 1}, eth1 , 1 }
  };

  router = std::make_unique<Router<IP4>>(Super_stack::inet().ip4_stacks(), routing_table);

  eth0.ip_obj().set_packet_forwarding(ip_forward);
  eth1.ip_obj().set_packet_forwarding(ip_forward);
  //laptop1.ip_obj().set_packet_forwarding(ip_forward);
  //internet_host.ip_obj().set_packet_forwarding(ip_forward);

  // Setup Conntracker
  ct = std::make_unique<Conntrack>();
  auto ct_in = [](IP4::IP_packet_ptr pkt, const Inet<IP4>& stack)->IP4::IP_packet_ptr {
    printf("Connection tracking on %s (%s)\n", stack.ip_addr().str().c_str(), stack.ifname().c_str());
    ct->in(*pkt);
    return pkt;
  };

  auto ct_confirm = [](IP4::IP_packet_ptr pkt, const Inet<IP4>& stack)->IP4::IP_packet_ptr {
    printf("CT Confirm on %s (%s)\n", stack.ip_addr().str().c_str(), stack.ifname().c_str());
    ct->confirm(*pkt);
    return pkt;
  };

  eth0.prerouting_chain().chain.push_back(ct_in);
  eth0.output_chain().chain.push_back(ct_in);
  eth0.postrouting_chain().chain.push_back(ct_confirm);
  eth0.input_chain().chain.push_back(ct_confirm);

  eth1.prerouting_chain().chain.push_back(ct_in);
  eth1.output_chain().chain.push_back(ct_in);
  eth1.postrouting_chain().chain.push_back(ct_confirm);
  eth1.input_chain().chain.push_back(ct_confirm);

  // Setup NAT (Masquerade)
  natty = std::make_unique<nat::NAPT>(ct.get());

  auto masq = [](IP4::IP_packet_ptr pkt, const Inet<IP4>& stack)->IP4::IP_packet_ptr {
    printf("Masq %s %s on %s (%s)\n",
      pkt->ip_src().str().c_str(), pkt->ip_dst().str().c_str(), stack.ip_addr().str().c_str(),
      stack.ifname().c_str());
    natty->masquerade(*pkt, stack);
    printf("-> %s %s\n", pkt->ip_src().str().c_str(), pkt->ip_dst().str().c_str());
    return pkt;
  };
  auto demasq = [](IP4::IP_packet_ptr pkt, const Inet<IP4>& stack)->IP4::IP_packet_ptr {
    printf("DeMasq %s %s on %s (%s)\n",
      pkt->ip_src().str().c_str(), pkt->ip_dst().str().c_str(), stack.ip_addr().str().c_str(),
      stack.ifname().c_str());
    natty->demasquerade(*pkt, stack);
    printf("-> %s %s\n", pkt->ip_src().str().c_str(), pkt->ip_dst().str().c_str());
    return pkt;
  };
  //eth0.prerouting_chain().chain.push_back(demasq);
  //eth0.postrouting_chain().chain.push_back(masq);
  eth1.prerouting_chain().chain.push_back(demasq);
  eth1.postrouting_chain().chain.push_back(masq);

  internet_host.tcp().listen(80, [](auto conn) {
    printf("Internet page received a new connection! (%s)\n", conn->to_string().c_str());
  });

  laptop1.tcp().connect({ internet_host.ip_addr(), 80 }, [](auto conn) {
    if(conn)
      printf("Laptop1 connected to internet web page! (%s)\n", conn->to_string().c_str());
  });
}
