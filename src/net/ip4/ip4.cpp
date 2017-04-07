// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
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

#define DEBUG // Allow debugging
#define DEBUG2 // Allow debug lvl 2

#include <net/ip4/ip4.hpp>
#include <net/ip4/packet_ip4.hpp>
#include <net/packet.hpp>
#include <statman>
#include <net/ip4/icmp4.hpp>

namespace net {

  const IP4::addr IP4::ADDR_ANY(0);
  const IP4::addr IP4::ADDR_BCAST(0xff,0xff,0xff,0xff);

  IP4::IP4(Stack& inet) noexcept :
  packets_rx_       {Statman::get().create(Stat::UINT64, inet.ifname() + ".ip4.packets_rx").get_uint64()},
  packets_tx_       {Statman::get().create(Stat::UINT64, inet.ifname() + ".ip4.packets_tx").get_uint64()},
  packets_dropped_  {Statman::get().create(Stat::UINT32, inet.ifname() + ".ip4.packets_dropped").get_uint32()},
  stack_            {inet},
  upstream_filter_  {this, &IP4::filter_upstream},
  downstream_filter_{this, &IP4::filter_downstream}
  {}


  IP4::IP_packet_ptr IP4::drop(IP_packet_ptr ptr, Direction direction, Drop_reason reason) {
    packets_dropped_++;

    if(drop_handler_)
      drop_handler_(std::move(ptr), direction, reason);

    return nullptr;
  }


  IP4::IP_packet_ptr IP4::filter_upstream(IP4::IP_packet_ptr packet)
  {
    IP4::Direction up = IP4::Direction::Upstream;

    // RFC-1122 3.2.1.1, Silently discard Version != 4
    if (UNLIKELY(not packet->is_ipv4()))
      return drop(std::move(packet), up, Drop_reason::Wrong_version);

    // RFC-1122 3.2.1.2, Verify IP checksum, silently discard bad dgram
    if (UNLIKELY(packet->compute_checksum() != 0))
      return drop(std::move(packet), up, Drop_reason::Wrong_checksum);

    // RFC-1122 3.2.1.3, Silently discard datagram with bad src addr
    // Here dropping if the source ip address is a multicast address or is this interface's broadcast address
    if (UNLIKELY(packet->ip_src().is_multicast() or packet->ip_src() == IP4::ADDR_BCAST or
      packet->ip_src() == stack_.broadcast_addr())) {
      return drop(std::move(packet), up, Drop_reason::Bad_source);
    }

    return packet;
  }


  IP4::IP_packet_ptr IP4::filter_downstream(IP4::IP_packet_ptr packet)
  {
    // RFC-1122 3.2.1.7, MUST NOT send packet with TTL of 0
    if (packet->ip_ttl() == 0)
      return drop(std::move(packet), Direction::Downstream, Drop_reason::TTL0);

    // RFC-1122 3.2.1.7, MUST NOT send packet addressed to 127.*
    if (packet->ip_dst().part(0) == 127)
      drop(std::move(packet), Direction::Downstream, Drop_reason::Bad_destination);

    return packet;
  }


  void IP4::receive(Packet_ptr pckt)
  {
    // Cast to IP4 Packet
    auto packet = static_unique_ptr_cast<net::PacketIP4>(std::move(pckt));

    debug("<IP4> received packet \n");
    debug2("\t* Source IP: %s Dest.IP: %s Type: 0x%x\n",
           packet->ip_src().str().c_str(),
           packet->ip_dst().str().c_str(),
           packet->ip_protocol());


    // Stat increment packets received
    packets_rx_++;

    packet = upstream_filter_(std::move(packet));
    if (UNLIKELY(packet == nullptr)) return;

    // Drop / forward if my ip address doesn't match dest. or broadcast
    if (UNLIKELY(packet->ip_dst() != local_ip()
                 and (packet->ip_dst() | stack_.netmask()) != ADDR_BCAST
                 and local_ip() != ADDR_ANY
                 and not stack_.is_loopback(packet->ip_dst()))) {

      if (forward_packet_) {
        forward_packet_(stack_, std::move(packet));
        debug("Packet forwarded \n");
      } else {
        debug("Packet dropped \n");
        drop(std::move(packet), Direction::Upstream, Drop_reason::Bad_destination);
      }

      return;
    }

    // Pass packet to it's respective protocol controller
    switch (packet->ip_protocol()) {
    case Protocol::ICMPv4:
      debug2("\t Type: ICMP\n");
      icmp_handler_(std::move(packet));
      break;
    case Protocol::UDP:
      debug2("\t Type: UDP\n");
      udp_handler_(std::move(packet));
      break;
    case Protocol::TCP:
      tcp_handler_(std::move(packet));
      debug2("\t Type: TCP\n");
      break;

    default:
      debug("\t Type: UNKNOWN %hhu. Dropping. \n", packet->ip_protocol());

      // Send ICMP error of type Destination Unreachable and code PROTOCOL
      // @note: If dest. is broadcast or multicast it should be dropped by now
      stack_.icmp().destination_unreachable(std::move(packet), icmp4::code::Dest_unreachable::PROTOCOL);

      drop(std::move(packet), Direction::Upstream, Drop_reason::Unknown_proto);
      break;
    }
  }

  void IP4::transmit(Packet_ptr pckt) {
    assert((size_t)pckt->size() > sizeof(header));

    auto ip4_pckt = static_unique_ptr_cast<PacketIP4>(std::move(pckt));

    ip4_pckt->make_flight_ready();

    ship(std::move(ip4_pckt));
  }

  void IP4::ship(Packet_ptr pckt)
  {
    auto ip4_pckt = static_unique_ptr_cast<PacketIP4>(std::move(pckt));

    // Send loopback packets right back
    if (UNLIKELY(stack_.is_loopback(ip4_pckt->ip_dst()))) {
      debug("<IP4> Destination address is loopback \n");
      IP4::receive(std::move(ip4_pckt));
      return;
    }

    addr next_hop;

    if (ip4_pckt->ip_dst() != IP4::ADDR_BCAST)
    {
      // Create local and target subnets
      addr target = ip4_pckt->ip_dst()  & stack_.netmask();
      addr local  = stack_.ip_addr() & stack_.netmask();

      // Compare subnets to know where to send packet
      next_hop = target == local ? ip4_pckt->ip_dst() : stack_.gateway();

      debug("<IP4 TOP> Next hop for %s, (netmask %s, local IP: %s, gateway: %s) == %s\n",
          ip4_pckt->ip_dst().str().c_str(),
          stack_.netmask().str().c_str(),
          stack_.ip_addr().str().c_str(),
          stack_.gateway().str().c_str(),
          next_hop.str().c_str());

    } else {
      next_hop = IP4::ADDR_BCAST;
    }

    // Filter illegal egress packets
    ip4_pckt = upstream_filter_(std::move(ip4_pckt));
    if (ip4_pckt == nullptr) return;

    // Stat increment packets transmitted
    packets_tx_++;

    debug("<IP4> Transmitting packet, layer begin: buf + %i\n", ip4_pckt->layer_begin() - ip4_pckt->buf());

    linklayer_out_(std::move(ip4_pckt), next_hop);
  }

} //< namespace net
