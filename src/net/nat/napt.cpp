// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017 Oslo and Akershus University College of Applied Sciences
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

#include <net/nat/napt.hpp>
#include <net/nat/nat.hpp>
#include <net/tcp/packet.hpp>
#include <net/ip4/packet_udp.hpp>

namespace net {
namespace nat {

NAPT::NAPT(std::shared_ptr<Conntrack> ct)
  : conntrack(std::move(ct))
{
  Expects(conntrack != nullptr);
}

void NAPT::masquerade(IP4::IP_packet& pkt, Stack& inet)
{
  switch(pkt.ip_protocol())
  {
    case Protocol::TCP:
      tcp_masq(pkt, inet);
      break;

    case Protocol::UDP:
      udp_masq(pkt, inet);
      break;

    case Protocol::ICMPv4:
      icmp_masq(pkt, inet);
      break;

    default:
      break;
  }
}

void NAPT::demasquerade(IP4::IP_packet& pkt, const Stack& inet)
{
  switch(pkt.ip_protocol())
  {
    case Protocol::TCP:
      tcp_demasq(pkt, inet);
      break;

    case Protocol::UDP:
      udp_demasq(pkt, inet);
      break;

    case Protocol::ICMPv4:
      icmp_demasq(pkt, inet);
      break;

    default:
      break;
  }
}

void NAPT::tcp_masq(IP4::IP_packet& p, Stack& inet)
{
  Expects(p.ip_protocol() == Protocol::TCP);
  auto& pkt = static_cast<tcp::Packet&>(p);
  Quadruple quad{pkt.source(), pkt.destination()};

  auto* entry = conntrack->get(quad, Protocol::TCP);

  if(not entry) return;

  // If the entry is mirrored, it's not masked yet
  if(entry->first.src == entry->second.dst)
  {
    // Get the TCP ports for the given stack
    const auto ip = inet.ip_addr();
    auto& ports = inet.tcp_ports()[ip];
    // Generate a new eph port and bind it
    auto port = ports.get_next_ephemeral();
    ports.bind(port);
    // Update the entry to have the new socket as second
    auto masq_sock = Socket{inet.ip_addr(), port};
    conntrack->update_entry(Protocol::TCP, entry->second, {entry->second.src, masq_sock});
  }

  // static source nat
  tcp_snat(pkt, entry->second.dst);
}

void NAPT::tcp_demasq(IP4::IP_packet& p, const Stack&)
{
  Expects(p.ip_protocol() == Protocol::TCP);
  auto& pkt = static_cast<tcp::Packet&>(p);
  Quadruple quad{pkt.source(), pkt.destination()};

  auto* entry = conntrack->get(quad, Protocol::TCP);

  if(not entry) return;

  // if the entry's in and out are not the same
  if(not entry->is_mirrored())
  {
    // static dest nat
    tcp_dnat(pkt, entry->first.src);
  }
}

void NAPT::udp_masq(IP4::IP_packet& p, Stack& inet)
{
  Expects(p.ip_protocol() == Protocol::UDP);
  auto& pkt = static_cast<PacketUDP&>(p);

  auto src = Socket{pkt.ip_src(), pkt.src_port()};
  auto dst = Socket{pkt.ip_dst(), pkt.dst_port()};
  Quadruple quad{src, dst};

  auto* entry = conntrack->get(quad, Protocol::UDP);

  if(not entry) return;

  if(entry->first.dst == entry->second.src)
  {
    // Get the UDP ports for the given stack
    const auto ip = inet.ip_addr();
    auto& ports = inet.udp_ports()[ip];
    // Generate a new eph port and bind it
    auto port = ports.get_next_ephemeral();
    ports.bind(port);
    // Update the entry to have the new socket as second
    auto masq_sock = Socket{inet.ip_addr(), port};
    conntrack->update_entry(Protocol::UDP, entry->second, {entry->second.src, masq_sock});
  }

  // static source nat
  udp_snat(pkt, entry->first.dst);
}

void NAPT::udp_demasq(IP4::IP_packet& p, const Stack&)
{
  Expects(p.ip_protocol() == Protocol::UDP);
  auto& pkt = static_cast<PacketUDP&>(p);

  auto src = Socket{pkt.ip_src(), pkt.src_port()};
  auto dst = Socket{pkt.ip_dst(), pkt.dst_port()};
  Quadruple quad{src, dst};

  auto* entry = conntrack->get(quad, Protocol::UDP);

  if(not entry) return;

  // if the entry's in and out are not the same
  if(not entry->is_mirrored())
  {
    // static dest nat
    udp_dnat(pkt, entry->second.src);
  }
}

void NAPT::icmp_masq(IP4::IP_packet& pkt, Stack& inet)
{
  // Not sure what ICMP masq means (yet)..
  Expects(pkt.ip_protocol() == Protocol::ICMPv4);

  auto quad = Conntrack::get_quadruple_icmp(pkt);

  auto* entry = conntrack->get(quad, Protocol::ICMPv4);

  if(not entry) return;

  // If the entry is mirrored, it's not masked yet
  if(entry->first.src == entry->second.dst)
  {
    // Update the entry to have the new socket as second (keep port)
    auto masq_sock = Socket{inet.ip_addr(), entry->second.dst.port()};
    conntrack->update_entry(Protocol::ICMPv4, entry->second, {entry->second.src, masq_sock});
  }

  // static source nat
  icmp_snat(pkt, entry->second.dst.address());
}

void NAPT::icmp_demasq(IP4::IP_packet& pkt, const Stack&)
{
  // Not sure what ICMP demasq means (yet)..
  Expects(pkt.ip_protocol() == Protocol::ICMPv4);

  auto quad = Conntrack::get_quadruple_icmp(pkt);

  auto* entry = conntrack->get(quad, Protocol::ICMPv4);

  if(not entry) return;

  // if the entry's in and out are not the same
  if(not entry->is_mirrored())
  {
    // static dest nat
    icmp_dnat(pkt, entry->second.src.address());
  }
}

}
}
