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

#include <net/nat/nat.hpp>
#include <net/inet_common.hpp> // checksum_adjust
#include <net/tcp/packet.hpp>
#include <net/ip4/packet_udp.hpp>

namespace net {
namespace nat {

// Helper functions
inline void recalc_ip_checksum(PacketIP4& pkt, ip4::Addr old_addr, ip4::Addr new_addr);
inline void recalc_tcp_sock(tcp::Packet& pkt, Socket osock, Socket nsock);
inline void recalc_tcp_addr(tcp::Packet& pkt, ip4::Addr old_addr, ip4::Addr new_addr);
inline void recalc_tcp_port(tcp::Packet& pkt, uint16_t old_port, uint16_t new_port);
inline void recalc_udp_sock(PacketUDP& pkt, Socket osock, Socket nsock);
inline void recalc_udp_addr(PacketUDP& pkt, ip4::Addr old_addr, ip4::Addr new_addr);
inline void recalc_udp_port(PacketUDP& pkt, uint16_t old_port, uint16_t new_port);

void snat(PacketIP4& pkt, Socket src_socket)
{
  switch(pkt.ip_protocol())
  {
    case Protocol::TCP:
      tcp_snat(pkt, src_socket);
      return;

    case Protocol::UDP:
      udp_snat(pkt, src_socket);
      return;

    default:
      return;
  }
}

void snat(PacketIP4& pkt, const ip4::Addr new_addr)
{
  switch(pkt.ip_protocol())
  {
    case Protocol::TCP:
      tcp_snat(pkt, new_addr);
      return;

    case Protocol::UDP:
      udp_snat(pkt, new_addr);
      return;

    case Protocol::ICMPv4:
      icmp_snat(pkt, new_addr);
      return;

    default:
      return;
  }
}

void snat(PacketIP4& pkt, const uint16_t new_port)
{
  switch(pkt.ip_protocol())
  {
    case Protocol::TCP:
      tcp_snat(pkt, new_port);
      return;

    case Protocol::UDP:
      udp_snat(pkt, new_port);
      return;

    default:
      return;
  }
}

void dnat(PacketIP4& pkt, Socket dst_socket)
{
  switch(pkt.ip_protocol())
  {
    case Protocol::TCP:
      tcp_dnat(pkt, dst_socket);
      return;

    case Protocol::UDP:
      udp_dnat(pkt, dst_socket);
      return;

    default:
      return;
  }
}

void dnat(PacketIP4& pkt, const ip4::Addr new_addr)
{
  switch(pkt.ip_protocol())
  {
    case Protocol::TCP:
      tcp_dnat(pkt, new_addr);
      return;

    case Protocol::UDP:
      udp_dnat(pkt, new_addr);
      return;

    case Protocol::ICMPv4:
      icmp_dnat(pkt, new_addr);
      return;

    default:
      return;
  }
}

void dnat(PacketIP4& pkt, const uint16_t new_port)
{
  switch(pkt.ip_protocol())
  {
    case Protocol::TCP:
      tcp_dnat(pkt, new_port);
      return;

    case Protocol::UDP:
      udp_dnat(pkt, new_port);
      return;

    default:
      return;
  }
}

// TCP SNAT //
void tcp_snat(PacketIP4& p, Socket new_sock)
{
  Expects(p.ip_protocol() == Protocol::TCP);
  auto& pkt = static_cast<tcp::Packet&>(p);
  // Recalc checksum
  recalc_tcp_sock(pkt, pkt.source(), new_sock);
  // change source socket
  pkt.set_source(new_sock);
}

void tcp_snat(PacketIP4& p, const ip4::Addr new_addr)
{
  Expects(p.ip_protocol() == Protocol::TCP);
  auto& pkt = static_cast<tcp::Packet&>(p);
  // recalc tcp address csum
  recalc_tcp_addr(pkt, pkt.ip_src(), new_addr);
  // change source address
  pkt.set_ip_src(new_addr);
}

void tcp_snat(PacketIP4& p, const uint16_t new_port)
{
  Expects(p.ip_protocol() == Protocol::TCP);
  auto& pkt = static_cast<tcp::Packet&>(p);
  // recalc tcp port
  recalc_tcp_port(pkt, pkt.src_port(), new_port);
  // change source port
  pkt.set_src_port(new_port);
}

// TCP DNAT //
void tcp_dnat(PacketIP4& p, Socket new_sock)
{
  Expects(p.ip_protocol() == Protocol::TCP);
  auto& pkt = static_cast<tcp::Packet&>(p);
  // Recalc checksum
  recalc_tcp_sock(pkt, pkt.destination(), new_sock);
  // change destination socket
  pkt.set_destination(new_sock);
}

void tcp_dnat(PacketIP4& p, const ip4::Addr new_addr)
{
  Expects(p.ip_protocol() == Protocol::TCP);
  auto& pkt = static_cast<tcp::Packet&>(p);
  // recalc tcp address csum
  recalc_tcp_addr(pkt, pkt.ip_dst(), new_addr);
  // change destination address
  pkt.set_ip_dst(new_addr);
}

void tcp_dnat(PacketIP4& p, const uint16_t new_port)
{
  Expects(p.ip_protocol() == Protocol::TCP);
  auto& pkt = static_cast<tcp::Packet&>(p);
  // recalc tcp port csum
  recalc_tcp_port(pkt, pkt.dst_port(), new_port);
  // change destination port
  pkt.set_dst_port(new_port);
}

// UDP SNAT //
void udp_snat(PacketIP4& p, Socket new_sock)
{
  Expects(p.ip_protocol() == Protocol::UDP);
  auto& pkt = static_cast<PacketUDP&>(p);
  auto old_sock = Socket{pkt.ip_src(), pkt.src_port()};
  // Recalc checksum
  recalc_udp_sock(pkt, old_sock, new_sock);
  // Set the value
  pkt.set_ip_src(new_sock.address());
  pkt.set_src_port(new_sock.port());
}

void udp_snat(PacketIP4& p, const ip4::Addr new_addr)
{
  Expects(p.ip_protocol() == Protocol::UDP);
  auto& pkt = static_cast<PacketUDP&>(p);
  // recalc udp addr csum
  recalc_udp_addr(pkt, pkt.ip_src(), new_addr);
  // change destination address
  pkt.set_ip_src(new_addr);
}

void udp_snat(PacketIP4& p, const uint16_t new_port)
{
  Expects(p.ip_protocol() == Protocol::UDP);
  auto& pkt = static_cast<PacketUDP&>(p);
  // recalc udp port csum
  recalc_udp_port(pkt, pkt.src_port(), new_port);
  // change source port
  pkt.set_src_port(new_port);
}

// UDP DNAT //
void udp_dnat(PacketIP4& p, Socket new_sock)
{
  Expects(p.ip_protocol() == Protocol::UDP);
  auto& pkt = static_cast<PacketUDP&>(p);
  auto old_sock = Socket{pkt.ip_dst(), pkt.dst_port()};

  // Recalc checksum
  recalc_udp_sock(pkt, old_sock, new_sock);

  // change destination
  pkt.set_ip_dst(new_sock.address());
  pkt.set_dst_port(new_sock.port());
}

void udp_dnat(PacketIP4& p, const ip4::Addr new_addr)
{
  Expects(p.ip_protocol() == Protocol::UDP);
  auto& pkt = static_cast<PacketUDP&>(p);
  // recalc udp addr csum
  recalc_udp_addr(pkt, pkt.ip_dst(), new_addr);
  // change destination address
  pkt.set_ip_dst(new_addr);
}

void udp_dnat(PacketIP4& p, const uint16_t new_port)
{
  Expects(p.ip_protocol() == Protocol::UDP);
  auto& pkt = static_cast<PacketUDP&>(p);
  // recalc udp port csum
  recalc_udp_port(pkt, pkt.dst_port(), new_port);
  // change destination port
  pkt.set_dst_port(new_port);
}

// ICMP NAT
void icmp_snat(PacketIP4& pkt, const ip4::Addr addr)
{
  // recalc IP checksum
  recalc_ip_checksum(pkt, pkt.ip_src(), addr);
  pkt.set_ip_src(addr);
}

void icmp_dnat(PacketIP4& pkt, const ip4::Addr addr)
{
  // recalc IP checksum
  recalc_ip_checksum(pkt, pkt.ip_dst(), addr);
  pkt.set_ip_dst(addr);
}

inline void recalc_ip_checksum(PacketIP4& pkt, ip4::Addr old_addr, ip4::Addr new_addr)
{
  auto ip_sum = pkt.ip_checksum();
  checksum_adjust(&ip_sum, &old_addr, &new_addr);
  pkt.set_ip_checksum(ip_sum);
}

inline void recalc_tcp_sock(tcp::Packet& pkt, Socket osock, Socket nsock)
{
  auto old_addr = osock.address();
  auto new_addr = nsock.address();

  // recalc IP checksum
  recalc_ip_checksum(pkt, old_addr, new_addr);

  auto tcp_sum = pkt.tcp_checksum();
  // recalc tcp address part
  checksum_adjust(&tcp_sum, &old_addr, &new_addr);
  // recalc tcp port part
  auto old_port = htons(osock.port());
  auto new_port = htons(nsock.port());
  checksum_adjust<uint16_t>(&tcp_sum, &old_port, &new_port);
  // set the new sum
  pkt.set_tcp_checksum(tcp_sum);
}

inline void recalc_tcp_addr(tcp::Packet& pkt, ip4::Addr old_addr, ip4::Addr new_addr)
{
  // recalc IP checksum
  recalc_ip_checksum(pkt, old_addr, new_addr);
  // recalc tcp address part
  auto tcp_sum = pkt.tcp_checksum();
  checksum_adjust(&tcp_sum, &old_addr, &new_addr);
  pkt.set_tcp_checksum(tcp_sum);
}

inline void recalc_tcp_port(tcp::Packet& pkt, uint16_t old_port, uint16_t new_port)
{
  // swap ports to network order
  old_port = htons(old_port);
  new_port = htons(new_port);
  // update TCP csum
  auto tcp_sum  = pkt.tcp_checksum();
  checksum_adjust<uint16_t>(&tcp_sum, &old_port, &new_port);
  pkt.set_tcp_checksum(tcp_sum);
}

inline void recalc_udp_sock(PacketUDP& pkt, Socket osock, Socket nsock)
{
  auto old_addr = osock.address();
  auto new_addr = nsock.address();

  // recalc IP checksum
  recalc_ip_checksum(pkt, old_addr, new_addr);

  // TODO: recalc UDP (not in use)
}

inline void recalc_udp_addr(PacketUDP& pkt, ip4::Addr old_addr, ip4::Addr new_addr)
{
  // recalc IP checksum
  recalc_ip_checksum(pkt, old_addr, new_addr);
  // TODO: recalc UDP checksum (psuedo header change)
}

inline void recalc_udp_port(PacketUDP&, uint16_t, uint16_t)
{
  // TODO: recalc UDP checksum
}

}
}
