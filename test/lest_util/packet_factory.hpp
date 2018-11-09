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

#pragma once
#ifndef TEST_PACKET_FACTORY_HPP
#define TEST_PACKET_FACTORY_HPP

#include <net/ethernet/header.hpp>
#include <net/buffer_store.hpp>
#include <net/packet.hpp>

#define BUFFER_CNT   128
#define BUFFER_SIZE 2048

#define PHYS_OFFSET     0
#define PACKET_CAPA  1514

using namespace net;
static net::Packet_ptr create_packet() noexcept
{
  static net::BufferStore bufstore(BUFFER_CNT, BUFFER_SIZE);
  auto* ptr = (net::Packet*) bufstore.get_buffer();
  new (ptr) net::Packet(PHYS_OFFSET, 0, PHYS_OFFSET + PACKET_CAPA, &bufstore);
  return net::Packet_ptr(ptr);
}

#include <net/ip4/packet_ip4.hpp>
static std::unique_ptr<net::PacketIP4> create_ip4_packet() noexcept
{
  auto pkt = create_packet();
  pkt->increment_layer_begin(sizeof(net::ethernet::Header));
  // IP4 Packet
  auto ip4 = net::static_unique_ptr_cast<net::PacketIP4> (std::move(pkt));
  return ip4;
}

static std::unique_ptr<net::PacketIP4> create_ip4_packet_init(ip4::Addr src, ip4::Addr dst) noexcept
{
  auto ip4 = create_ip4_packet();
  ip4->init();
  ip4->set_ip_total_length(ip4->size());
  ip4->set_ip_src(src);
  ip4->set_ip_dst(dst);
  return ip4;
}

#include <net/ip6/packet_ip6.hpp>
static std::unique_ptr<net::PacketIP6> create_ip6_packet() noexcept
{
  auto pkt = create_packet();
  pkt->increment_layer_begin(sizeof(net::ethernet::Header));
  // IP6 Packet
  auto ip6 = net::static_unique_ptr_cast<net::PacketIP6> (std::move(pkt));
  return ip6;
}

static std::unique_ptr<net::PacketIP6> create_ip6_packet_init(ip6::Addr src, ip6::Addr dst) noexcept
{
  auto ip6 = create_ip6_packet();
  ip6->init();
  ip6->set_ip_src(src);
  ip6->set_ip_dst(dst);
  return ip6;
}

#include <net/tcp/packet.hpp>
static std::unique_ptr<net::tcp::Packet> create_tcp_packet() noexcept
{
  auto ip4 = create_ip4_packet();
  ip4->init(Protocol::TCP);
  ip4->set_ip_total_length(ip4->size());
  auto tcp = net::static_unique_ptr_cast<net::tcp::Packet> (std::move(ip4));
  return tcp;
}

static std::unique_ptr<net::tcp::Packet> create_tcp_packet_init(Socket src, Socket dst) noexcept
{
  auto tcp = create_tcp_packet();
  tcp->init();
  tcp->set_ip_total_length(tcp->size());
  tcp->set_source(src);
  tcp->set_destination(dst);
  return tcp;
}

#include <net/udp/packet_udp.hpp>
static std::unique_ptr<net::PacketUDP> create_udp_packet_init(Socket src, Socket dst) noexcept
{
  auto ip4 = create_ip4_packet();
  ip4->init(Protocol::UDP);
  auto udp = net::static_unique_ptr_cast<net::PacketUDP> (std::move(ip4));
  udp->init(src.port(), dst.port());
  udp->set_ip_src(src.address().v4());
  udp->set_ip_dst(dst.address().v4());
  return udp;
}

#endif
