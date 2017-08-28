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

  route->ip_obj().ship(std::move(pckt));
}

void Service::start()
{
  INFO("NAT Test", "Setting up enviornment to simulate a home router");
  auto& eth0 = Inet4::ifconfig<0>(
    { 10, 1, 0, 1 }, { 255, 255, 255, 0 }, { 10, 0, 0, 1 });

  auto& eth1 = Inet4::ifconfig<1>(
    { 192, 1, 0, 1 }, { 255, 255, 255, 0 }, { 10, 0, 0, 1 });

  auto& laptop1 = Inet4::ifconfig<2>(
    { 10, 1, 0, 10 }, { 255, 255, 255, 0 }, eth0.ip_addr());

  auto& internet_host = Inet4::ifconfig<3>(
    { 192, 1, 0, 192 }, { 255, 255, 255, 0 }, eth1.ip_addr());


  INFO("NAT Test", "Setup routing between eth0 and eth1");
  Router<IP4>::Routing_table routing_table{
    {{10, 1, 0, 0 }, { 255, 255, 255, 0}, {10, 1, 0, 1}, eth0 , 1 },
    {{192, 1, 0, 0 }, { 255, 255, 255, 0}, {192, 1, 0, 1}, eth1 , 1 }
  };

  router = std::make_unique<Router<IP4>>(Super_stack::inet().ip4_stacks(), routing_table);
  eth0.ip_obj().set_packet_forwarding(ip_forward);
  eth1.ip_obj().set_packet_forwarding(ip_forward);

  // Setup Conntracker
  INFO("NAT Test", "Enable Conntrack on eth0 and eth1");
  ct = std::make_shared<Conntrack>();
  eth0.enable_conntrack(ct);
  eth1.enable_conntrack(ct);

  // Setup NAT (Masquerade)
  natty = std::make_unique<nat::NAPT>(ct);

  auto masq = [](IP4::IP_packet& pkt, Inet<IP4>& stack)->auto {
    //printf("Masq %s %s on %s (%s)\n",
    //  pkt.ip_src().str().c_str(), pkt.ip_dst().str().c_str(), stack.ip_addr().str().c_str(),
    //  stack.ifname().c_str());
    natty->masquerade(pkt, stack);
    //printf("-> %s %s\n", pkt.ip_src().str().c_str(), pkt.ip_dst().str().c_str());
    return Filter_verdict::ACCEPT;
  };
  auto demasq = [](IP4::IP_packet& pkt, Inet<IP4>& stack)->auto {
    //printf("DeMasq %s %s on %s (%s)\n",
    //  pkt.ip_src().str().c_str(), pkt.ip_dst().str().c_str(), stack.ip_addr().str().c_str(),
    //  stack.ifname().c_str());
    natty->demasquerade(pkt, stack);
    //printf("-> %s %s\n", pkt.ip_src().str().c_str(), pkt.ip_dst().str().c_str());
    return Filter_verdict::ACCEPT;
  };

  INFO("NAT Test", "Enable MASQUERADE on eth1");
  eth1.prerouting_chain().chain.push_back(demasq);
  eth1.postrouting_chain().chain.push_back(masq);

  // Open TCP 80 on internet
  internet_host.tcp().listen(80, [&eth1] (auto conn) {
    INFO("TCP MASQ", "Internet host receiving connection");
    CHECKSERT(conn->remote().address() == eth1.ip_addr(),
      "Received connection from (what appears to be) my gateway! (%s)", conn->to_string().c_str());
    conn->on_read(1024, [](auto buf, auto n) {
      auto str = std::string{reinterpret_cast<const char*>(buf.get()), n};
      CHECKSERT(str == "Testing NAT", "Data from laptop is received");

      printf("SUCCESS\n");
    });
  });

  // Connect to internet from laptop in local network
  auto website = Socket{internet_host.ip_addr(), 80};
  laptop1.tcp().connect(website, [website] (auto conn) {
    INFO("TCP MASQ", "Laptop connecting out");
    assert(conn);
    CHECKSERT(conn->remote() == website,
      "Laptop1 connected to internet! (%s)", conn->to_string().c_str());

    conn->write("Testing NAT");
  });
}
