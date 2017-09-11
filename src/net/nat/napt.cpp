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

#define NAPT_DEBUG 1
#ifdef NAPT_DEBUG
#define NATDBG(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define NATDBG(fmt, ...) /* fmt */
#endif

namespace net {
namespace nat {

NAPT::NAPT(std::shared_ptr<Conntrack> ct)
  : conntrack(std::move(ct))
{
  Expects(conntrack != nullptr);
}

void NAPT::masquerade(IP4::IP_packet& pkt, Stack& inet, Conntrack::Entry_ptr entry)
{
  Expects(entry);
  const auto ip = inet.ip_addr();
  switch(pkt.ip_protocol())
  {
    case Protocol::TCP:
    {
      // Get the TCP ports for the given stack
      auto& ports = inet.tcp_ports()[ip];
      auto socket = masq(entry, ip, ports);
      // static source nat
      tcp_snat(pkt, socket);
      break;
    }

    case Protocol::UDP:
    {
      // Get the UDP ports for the given stack
      auto& ports = inet.udp_ports()[ip];
      auto socket = masq(entry, ip, ports);
      // static source nat
      udp_snat(pkt, socket);
      break;
    }

    case Protocol::ICMPv4:
    {
      // If the entry is mirrored, it's not masked yet
      if(entry->first.src == entry->second.dst)
      {
        // Update the entry to have the new socket as second (keep port)
        auto masq_sock = Socket{ip, entry->second.dst.port()};
        conntrack->update_entry(Protocol::ICMPv4, entry->second, {entry->second.src, masq_sock});
      }
      // static source nat
      icmp_snat(pkt, entry->second.dst.address());
      break;
    }

    default:
      break;
  }
}

void NAPT::demasquerade(IP4::IP_packet& pkt, const Stack&, Conntrack::Entry_ptr entry)
{
  Expects(entry);

  if(entry->is_mirrored())
    return;

  switch(pkt.ip_protocol())
  {
    case Protocol::TCP:
      tcp_dnat(pkt, entry->first.src);
      break;

    case Protocol::UDP:
      udp_dnat(pkt, entry->first.src);
      break;

    case Protocol::ICMPv4:
      icmp_dnat(pkt, entry->first.src.address());
      break;

    default:
      break;
  }
}

Socket NAPT::masq(Conntrack::Entry_ptr entry, const ip4::Addr addr, Port_util& ports)
{
  Expects(entry->proto != Protocol::ICMPv4);

  // If the entry is mirrored, it's not masked yet
  if(entry->first.src == entry->second.dst)
  {
    // Generate a new eph port and bind it
    auto port = ports.get_next_ephemeral();
    ports.bind(port);

    // Update the entry to have the new socket as second
    auto masq_sock = Socket{addr, port};
    auto updated = conntrack->update_entry(
      entry->proto, entry->second, {entry->second.src, masq_sock});

    // Setup to unbind port on entry close
    auto on_close = [&ports, port](Conntrack::Entry_ptr){ ports.unbind(port); };
    updated->on_close = on_close;
  }

  return entry->second.dst;
}

void NAPT::dnat(IP4::IP_packet& p, Conntrack::Entry_ptr entry, Socket socket)
{
  Expects(entry);
  NATDBG("<NAPT> DNAT: %s => %s\n",
        entry->to_string().c_str(), socket.to_string().c_str());

  const auto proto = p.ip_protocol();
  switch(proto)
  {
    case Protocol::TCP:
    {
      if(entry->second.src != socket)
        conntrack->update_entry(proto, entry->second, {socket, entry->second.dst});

      tcp_dnat(p, socket);
      return;
    }
    case Protocol::UDP:
    {
      if(entry->second.src != socket)
        conntrack->update_entry(proto, entry->second, {socket, entry->second.dst});

      udp_dnat(p, socket);
      return;
    }
    case Protocol::ICMPv4:
    {
      // do not support Socket
      return;
    }
    default:
      return;
  }
}

void NAPT::snat(IP4::IP_packet& p, Conntrack::Entry_ptr entry)
{
  Expects(entry);
  switch(p.ip_protocol())
  {
    case Protocol::TCP:
    {
      const auto quad = Conntrack::get_quadruple(p);
      if(quad == entry->second)
      {
        NATDBG("<NAPT> Found SNAT target: %s => %s\n",
          entry->to_string().c_str(), entry->first.dst.to_string().c_str());
        tcp_snat(p, entry->first.dst);
      }
      return;
    }
    case Protocol::UDP:
    {
      const auto quad = Conntrack::get_quadruple(p);
      if(quad == entry->second)
      {
        NATDBG("<NAPT> Found SNAT target: %s => %s\n",
          entry->to_string().c_str(), entry->first.dst.to_string().c_str());
        udp_dnat(p, entry->first.dst);
      }
      return;
    }
    case Protocol::ICMPv4:
    {
      // do not support Socket
      return;
    }
    default:
      return;
  }
}

}
}
