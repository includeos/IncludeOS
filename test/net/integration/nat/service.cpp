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

void test_finished() {
  static int i = 0;
  if (++i == 2) printf("SUCCESS\n");
}

void ip_forward(Inet<IP4>& stack,  IP4::IP_packet_ptr pckt) {
  // Packet could have been erroneously moved prior to this call
  if (not pckt)
    return;

  Inet<IP4>* route = router->get_first_interface(pckt->ip_dst());

  if (not route){
    INFO("ip_fwd", "No route found for %s dropping", pckt->ip_dst().to_string().c_str());
    return;
  }

  route->ip_obj().ship(std::move(pckt));
}

void Service::start()
{
  INFO("NAT Test", "Setting up enviornment to simulate a home router");
  static auto& eth0 = Inet4::ifconfig<0>(
    { 10, 1, 0, 1 }, { 255, 255, 0, 0 }, { 10, 0, 0, 1 });

  static auto& eth1 = Inet4::ifconfig<1>(
    { 192, 1, 0, 1 }, { 255, 255, 255, 0 }, { 10, 0, 0, 1 });

  static auto& laptop1 = Inet4::ifconfig<2>(
    { 10, 1, 0, 10 }, { 255, 255, 255, 0 }, eth0.ip_addr());

  static auto& internet_host = Inet4::ifconfig<3>(
    { 192, 1, 0, 192 }, { 255, 255, 255, 0 }, eth1.ip_addr());

  static auto& mail_server = Inet4::ifconfig<4>(
    { 10, 1, 10, 20 }, { 255, 255, 255, 0 }, eth0.ip_addr());


  INFO("NAT Test", "Setup routing between eth0 and eth1");
  Router<IP4>::Routing_table routing_table{
    {{10, 1, 0, 0 }, { 255, 255, 0, 0}, {10, 1, 0, 1}, eth0 , 1 },
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

  auto masq = [](IP4::IP_packet& pkt, Inet<IP4>& stack, Conntrack::Entry_ptr entry)->auto {
    natty->masquerade(pkt, stack, entry);
    return Filter_verdict::ACCEPT;
  };
  auto demasq = [](IP4::IP_packet& pkt, Inet<IP4>& stack, Conntrack::Entry_ptr entry)->auto {
    natty->demasquerade(pkt, stack, entry);
    return Filter_verdict::ACCEPT;
  };

  INFO("NAT Test", "Enable MASQUERADE on eth1");
  eth1.prerouting_chain().chain.push_back(demasq);
  eth1.postrouting_chain().chain.push_back(masq);

  // Open TCP 80 on internet
  internet_host.tcp().listen(80, [] (auto conn)
  {
    INFO("TCP MASQ", "Internet host receiving connection - %s", conn->to_string().c_str());
    CHECKSERT(conn->remote().address() == eth1.ip_addr(),
      "Received connection from (what appears to be) my gateway - %s", conn->remote().to_string().c_str());

    conn->on_read(1024, [](auto buf, auto n)
    {
      const auto str = std::string{reinterpret_cast<const char*>(buf.get()), n};
      CHECKSERT(str == "Testing MASQ", "Data from laptop is received - \"%s\"", str.c_str());

      test_finished();
    });
  });

  // Connect to internet from laptop in local network
  static const auto website_socket = Socket{internet_host.ip_addr(), 80};
  laptop1.tcp().connect(website_socket, [] (auto conn)
  {
    assert(conn);
    INFO("TCP MASQ", "Laptop connected out - %s", conn->to_string().c_str());
    CHECKSERT(conn->remote() == website_socket,
      "Laptop1 connected to internet - %s", conn->remote().to_string().c_str());

    conn->write("Testing MASQ");
  });


  INFO("NAT Test", "Setup DNAT on eth0");
  static const Socket LOCAL_SERVER{mail_server.ip_addr(), 8080};
  static const ip4::Addr DNAT_ADDR{eth1.ip_addr()};
  static const uint16_t DNAT_PORT{3389};
  // DNAT all TCP on dst_port==DNAT_PORT to LOCAL_SERVER
  auto dnat_rule = [](IP4::IP_packet& pkt, Inet<IP4>& stack, Conntrack::Entry_ptr entry)->auto
  {
    if(pkt.ip_protocol() == Protocol::TCP)
    {
      auto& tcp = static_cast<tcp::Packet&>(pkt);
      if(tcp.dst_port() == DNAT_PORT)
        natty->dnat(pkt, entry, LOCAL_SERVER);
    }
    return Filter_verdict::ACCEPT;
  };
  // SNAT all packets that comes in return that has been DNAT
  auto snat_translate = [](IP4::IP_packet& pkt, Inet<IP4>& stack, Conntrack::Entry_ptr entry)->auto
  {
    natty->snat(pkt, entry);
    return Filter_verdict::ACCEPT;
  };

  eth0.prerouting_chain().chain.push_back(dnat_rule);
  eth0.postrouting_chain().chain.push_back(snat_translate);

  mail_server.tcp().listen(8080, [] (auto conn)
  {
    INFO("TCP DNAT", "Mail server received connection - %s", conn->to_string().c_str());
    CHECKSERT(conn->remote().address() != eth1.ip_addr(),
      "Received non SNAT connection - %s", conn->remote().to_string().c_str());

    conn->on_read(1024, [conn](auto buf, auto n)
    {
      const auto str = std::string{reinterpret_cast<const char*>(buf.get()), n};
      CHECKSERT(str == "Testing DNAT", "Data from laptop is received - \"%s\"", str.c_str());

      test_finished();
    });
  });

  static auto mail_socket = Socket{DNAT_ADDR, DNAT_PORT};
  laptop1.tcp().connect(mail_socket, [] (auto conn)
  {
    assert(conn);
    INFO("TCP DNAT", "Laptop1 connected to %s", conn->to_string().c_str());
    CHECKSERT(conn->remote() == mail_socket,
      "Laptop1 connected to mail_server - %s", conn->remote().to_string().c_str());

    conn->write("Testing DNAT");
  });

}
