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

NAPT::NAPT(const Conntrack* ct)
  : conntrack(ct)
{}

void NAPT::masquerade(IP4::IP_packet& pkt, const Stack& inet)
{
  switch(pkt.ip_protocol())
  {
    case Protocol::TCP:
      tcp_masq(pkt, inet);
      break;

    case Protocol::UDP:
      udp_masq(pkt, inet);
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

    default:
      break;
  }
}

void NAPT::tcp_masq(IP4::IP_packet& p, const Stack& inet)
{
  Expects(p.ip_protocol() == Protocol::TCP);
  auto& pkt = static_cast<tcp::Packet&>(p);
  Quadruple quad{pkt.source(), pkt.destination()};

  auto* entry = conntrack->out(quad, Protocol::TCP);
  Expects(entry);

  if(entry->in.dst == entry->out.src)
  {
    // Generate a new port
    auto port = tcp_ports.get_next_ephemeral();
    tcp_ports.bind(port);

    entry->in.dst = {inet.ip_addr(), port};
  }

  // static source nat
  tcp_snat(pkt, entry->in.dst);
}

void NAPT::tcp_demasq(IP4::IP_packet& p, const Stack&)
{
  Expects(p.ip_protocol() == Protocol::TCP);
  auto& pkt = static_cast<tcp::Packet&>(p);
  Quadruple quad{pkt.source(), pkt.destination()};

  auto* entry = conntrack->in(quad, Protocol::TCP);
  Expects(entry);

  // if the entry's in and out are not the same
  if(not entry->is_mirrored())
  {
    // static dest nat
    tcp_dnat(pkt, entry->out.src);
  }
}

void NAPT::udp_masq(IP4::IP_packet& p, const Stack& inet)
{
  Expects(p.ip_protocol() == Protocol::UDP);
  auto& pkt = static_cast<PacketUDP&>(p);

  auto src = Socket{pkt.ip_src(), pkt.src_port()};
  auto dst = Socket{pkt.ip_dst(), pkt.dst_port()};
  Quadruple quad{src, dst};

  auto* entry = conntrack->out(quad, Protocol::UDP);

  if(not entry) return;

  if(entry->in.dst == entry->out.src)
  {
    // Generate a new port
    auto port = udp_ports.get_next_ephemeral();
    udp_ports.bind(port);

    entry->in.dst = {inet.ip_addr(), port};
  }

  // static source nat
  udp_snat(pkt, entry->in.dst);
}

void NAPT::udp_demasq(IP4::IP_packet& p, const Stack&)
{
  Expects(p.ip_protocol() == Protocol::UDP);
  auto& pkt = static_cast<PacketUDP&>(p);

  auto src = Socket{pkt.ip_src(), pkt.src_port()};
  auto dst = Socket{pkt.ip_dst(), pkt.dst_port()};
  Quadruple quad{src, dst};

  auto* entry = conntrack->in(quad, Protocol::UDP);
  Expects(entry);

  // if the entry's in and out are not the same
  if(not entry->is_mirrored())
  {
    // static dest nat
    udp_dnat(pkt, entry->out.src);
  }
}

}
}
