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
#include <net/inet>

//#define NAPT_DEBUG 1
#ifdef NAPT_DEBUG
#define NATDBG(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define NATDBG(fmt, ...) /* fmt */
#endif

namespace net {
namespace nat {

inline bool is_snat(const Conntrack::Entry_ptr entry)
{ return entry->first.src != entry->second.dst; }

inline bool is_dnat(const Conntrack::Entry_ptr entry)
{ return entry->first.dst != entry->second.src; }

// Helpers to update the entry (if needed)
inline void update_dnat(Conntrack& ct, Conntrack::Entry_ptr entry, Socket socket)
{
  if(entry->second.src != socket) {
    ct.update_entry(entry->proto, entry->second, {
      socket, // replace return socket
      entry->second.dst
    });
  }
}

inline void update_dnat(Conntrack& ct, Conntrack::Entry_ptr entry, ip4::Addr addr)
{
  if(entry->second.src.address().v4() != addr) {
    ct.update_entry(entry->proto, entry->second, {
      {addr, entry->second.src.port()}, // change return addr but keep port
      entry->second.dst
    });
  }
}

inline void update_dnat(Conntrack& ct, Conntrack::Entry_ptr entry, uint16_t port)
{
  if(entry->second.src.port() != port) {
    ct.update_entry(entry->proto, entry->second, {
      {entry->second.src.address(), port}, // keep return address but change port
      entry->second.dst
    });
  }
}

inline void update_snat(Conntrack& ct, Conntrack::Entry_ptr entry, Socket socket)
{
  if(entry->second.dst != socket) {
    ct.update_entry(entry->proto, entry->second, {
      entry->second.src,
      socket // replace dst socket
    });
  }
}

inline void update_snat(Conntrack& ct, Conntrack::Entry_ptr entry, ip4::Addr addr)
{
  if(entry->second.dst.address().v4() != addr) {
    ct.update_entry(entry->proto, entry->second, {
      entry->second.src,
      {addr, entry->second.dst.port()} // change dst address but keep port
    });
  }
}

inline void update_snat(Conntrack& ct, Conntrack::Entry_ptr entry, uint16_t port)
{
  if(entry->second.dst.port() != port) {
    ct.update_entry(entry->proto, entry->second, {
      entry->second.src,
      {entry->second.dst.address(), port} // keep dst address but change port
    });
  }
}


NAPT::NAPT(std::shared_ptr<Conntrack> ct)
  : conntrack(std::move(ct))
{
  Expects(conntrack != nullptr);
}

void NAPT::masquerade(IP4::IP_packet& pkt, Stack& inet, Conntrack::Entry_ptr entry)
{
  if (UNLIKELY(entry == nullptr)) return;

  const auto ip = inet.ip_addr();
  switch(pkt.ip_protocol())
  {
    case Protocol::TCP:
    {
      // Get the TCP ports for the given stack
      auto& ports = inet.tcp_ports()[ip];
      auto socket = masq(entry, ip, ports);
      NATDBG("<NAPT> MASQ: %s => %s\n",
        entry->to_string().c_str(), socket.to_string().c_str());
      // static source nat
      tcp_snat(pkt, socket);
      break;
    }

    case Protocol::UDP:
    {
      // Get the UDP ports for the given stack
      auto& ports = inet.udp_ports()[ip];
      auto socket = masq(entry, ip, ports);
      NATDBG("<NAPT> MASQ: %s => %s\n",
        entry->to_string().c_str(), socket.to_string().c_str());
      // static source nat
      udp_snat(pkt, socket);
      break;
    }

    case Protocol::ICMPv4:
    {
      // If the entry is mirrored, it's not masked yet
      if(not is_snat(entry))
      {
        // Update the entry to have the new socket as second (keep port)
        auto masq_sock = Socket{ip, entry->second.dst.port()};
        conntrack->update_entry(Protocol::ICMPv4, entry->second, {entry->second.src, masq_sock});
      }
      NATDBG("<NAPT> MASQ: %s => %s\n",
        entry->to_string().c_str(), entry->second.dst.to_string().c_str());
      // static source nat
      icmp_snat(pkt, entry->second.dst.address().v4());
      break;
    }

    default:
      break;
  }
}

void NAPT::demasquerade(IP4::IP_packet& pkt, const Stack&, Conntrack::Entry_ptr entry)
{
  // unknown protocols aren't tracked, so exit
  if (UNLIKELY(entry == nullptr)) return;

  NATDBG("<NAPT> DEMASQ BEFORE IS_SNAT: %s\n",
        entry->to_string().c_str());

  if(not is_snat(entry))
    return;

  NATDBG("<NAPT> DEMASQ: %s => %s\n",
        entry->to_string().c_str(), entry->first.src.to_string().c_str());
  switch(pkt.ip_protocol())
  {
    case Protocol::TCP:
      tcp_dnat(pkt, entry->first.src);
      break;

    case Protocol::UDP:
      udp_dnat(pkt, entry->first.src);
      break;

    case Protocol::ICMPv4:
      icmp_dnat(pkt, entry->first.src.address().v4());
      break;

    default:
      break;
  }
}

Socket NAPT::masq(Conntrack::Entry_ptr entry, const ip4::Addr addr, Port_util& ports)
{
  Expects(entry->proto != Protocol::ICMPv4);

  // If the entry is mirrored, it's not masked yet
  if(not is_snat(entry))
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

void NAPT::dnat(IP4::IP_packet& p, Conntrack::Entry_ptr entry, const Socket socket)
{
  if (UNLIKELY(entry == nullptr)) return;

  NATDBG("<NAPT> DNAT: %s => %s\n",
        entry->to_string().c_str(), socket.to_string().c_str());

  const auto proto = p.ip_protocol();
  switch(proto)
  {
    case Protocol::TCP:
    {
      update_dnat(*conntrack, entry, socket);
      tcp_dnat(p, socket);
      return;
    }
    case Protocol::UDP:
    {
      update_dnat(*conntrack, entry, socket);
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

void NAPT::dnat(IP4::IP_packet& p, Conntrack::Entry_ptr entry, const ip4::Addr addr)
{
  if (UNLIKELY(entry == nullptr)) return;

  NATDBG("<NAPT> DNAT: %s => addr %s\n",
        entry->to_string().c_str(), addr.to_string().c_str());

  const auto proto = p.ip_protocol();

  update_dnat(*conntrack, entry, addr);

  switch(proto)
  {
    case Protocol::TCP:
      tcp_dnat(p, addr);
      return;

    case Protocol::UDP:
      udp_dnat(p, addr);
      return;

    case Protocol::ICMPv4:
      icmp_dnat(p, addr);
      return;

    default:
      return;
  }
}

void NAPT::dnat(IP4::IP_packet& p, Conntrack::Entry_ptr entry, const uint16_t port)
{
  if (UNLIKELY(entry == nullptr)) return;

  NATDBG("<NAPT> DNAT: %s => port %d\n",
        entry->to_string().c_str(), port);

  const auto proto = p.ip_protocol();

  update_dnat(*conntrack, entry, port);

  switch(proto)
  {
    case Protocol::TCP:
      tcp_dnat(p, port);
      return;

    case Protocol::UDP:
      udp_dnat(p, port);
      return;

    case Protocol::ICMPv4:
      // port not supported
      return;

    default:
      return;
  }
}

void NAPT::dnat(IP4::IP_packet& p, Conntrack::Entry_ptr entry)
{
  if (UNLIKELY(entry == nullptr)) return;

  if(not is_snat(entry)) // The entry has not been SNAT
    return;

  switch(p.ip_protocol())
  {
    case Protocol::TCP:
    {
      if(Conntrack::get_quadruple(p).dst == entry->second.dst) // assume reply
      {
        NATDBG("<NAPT> Found DNAT target: %s => %s\n",
          entry->to_string().c_str(), entry->first.src.to_string().c_str());
        tcp_dnat(p, entry->first.src); // TODO: currently rewrites full socket
      }
      return;
    }
    case Protocol::UDP:
    {
      if(Conntrack::get_quadruple(p).dst == entry->second.dst) // assume reply
      {
        NATDBG("<NAPT> Found DNAT target: %s => %s\n",
          entry->to_string().c_str(), entry->first.src.to_string().c_str());
        udp_dnat(p, entry->first.src); // TODO: currently rewrites full socket
      }
      return;
    }
    case Protocol::ICMPv4:
    {
      if(Conntrack::get_quadruple_icmp(p).dst == entry->second.dst) // assume reply
      {
        NATDBG("<NAPT> Found DNAT target: %s => %s\n",
          entry->to_string().c_str(), entry->first.src.address().to_string().c_str());
        icmp_dnat(p, entry->first.src.address().v4());
      }
      return;
    }
    default:
      return;
  }
}

void NAPT::snat(IP4::IP_packet& p, Conntrack::Entry_ptr entry, const Socket socket)
{
  if (UNLIKELY(entry == nullptr)) return;

  NATDBG("<NAPT> SNAT: %s => %s\n",
        entry->to_string().c_str(), socket.to_string().c_str());

  const auto proto = p.ip_protocol();
  switch(proto)
  {
    case Protocol::TCP:
    {
      update_snat(*conntrack, entry, socket);
      tcp_snat(p, socket);
      return;
    }
    case Protocol::UDP:
    {
      update_snat(*conntrack, entry, socket);
      udp_snat(p, socket);
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

void NAPT::snat(IP4::IP_packet& p, Conntrack::Entry_ptr entry, const ip4::Addr addr)
{
  if (UNLIKELY(entry == nullptr)) return;
  NATDBG("<NAPT> SNAT: %s => addr %s\n",
        entry->to_string().c_str(), addr.to_string().c_str());

  const auto proto = p.ip_protocol();

  update_snat(*conntrack, entry, addr);

  switch(proto)
  {
    case Protocol::TCP:
      tcp_snat(p, addr);
      return;

    case Protocol::UDP:
      udp_snat(p, addr);
      return;

    case Protocol::ICMPv4:
      icmp_snat(p, addr);
      return;

    default:
      return;
  }
}

void NAPT::snat(IP4::IP_packet& p, Conntrack::Entry_ptr entry, const uint16_t port)
{
  if (UNLIKELY(entry == nullptr)) return;
  NATDBG("<NAPT> SNAT: %s => port %d\n",
        entry->to_string().c_str(), port);

  const auto proto = p.ip_protocol();

  update_snat(*conntrack, entry, port);

  switch(proto)
  {
    case Protocol::TCP:
      tcp_snat(p, port);
      return;

    case Protocol::UDP:
      udp_snat(p, port);
      return;

    case Protocol::ICMPv4:
      // port not supported
      return;

    default:
      return;
  }
}

void NAPT::snat(IP4::IP_packet& p, Conntrack::Entry_ptr entry)
{
  if (UNLIKELY(entry == nullptr)) return;

  if(not is_dnat(entry)) // The entry has not been DNAT
    return;

  switch(p.ip_protocol())
  {
    case Protocol::TCP:
    {
      if(Conntrack::get_quadruple(p).src == entry->second.src) // assume reply
      {
        NATDBG("<NAPT> Found SNAT target: %s => %s\n",
          entry->to_string().c_str(), entry->first.dst.to_string().c_str());
        tcp_snat(p, entry->first.dst); // TODO: currently rewrites full socket
      }
      return;
    }
    case Protocol::UDP:
    {
      if(Conntrack::get_quadruple(p).src == entry->second.src) // assume reply
      {
        NATDBG("<NAPT> Found SNAT target: %s => %s\n",
          entry->to_string().c_str(), entry->first.dst.to_string().c_str());
        udp_snat(p, entry->first.dst); // TODO: currently rewrites full socket
      }
      return;
    }
    case Protocol::ICMPv4:
    {
      if(Conntrack::get_quadruple_icmp(p).src == entry->second.src) // assume reply
      {
        NATDBG("<NAPT> Found SNAT target: %s => %s\n",
          entry->to_string().c_str(), entry->first.dst.address().to_string().c_str());
        icmp_snat(p, entry->first.dst.address().v4());
      }
      return;
    }
    default:
      return;
  }
}

}
}
