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

namespace net {

  const IP4::addr IP4::ADDR_ANY(0);
  const IP4::addr IP4::ADDR_BCAST(0xff,0xff,0xff,0xff);

  IP4::IP4(Stack& inet) noexcept:
  packets_rx_       {Statman::get().create(Stat::UINT64, inet.ifname() + ".ip4.packets_rx").get_uint64()},
  packets_tx_       {Statman::get().create(Stat::UINT64, inet.ifname() + ".ip4.packets_tx").get_uint64()},
  packets_dropped_  {Statman::get().create(Stat::UINT32, inet.ifname() + ".ip4.packets_dropped").get_uint32()},
  stack_            {inet}
{
  // Default gateway is addr 1 in the subnet.
  // const uint32_t DEFAULT_GATEWAY = htonl(1);
  // gateway_.whole = (local_ip_.whole & netmask_.whole) | DEFAULT_GATEWAY;
}

  void IP4::bottom(Packet_ptr pckt) {
    debug2("<IP4 handler> got the data.\n");
    // Cast to IP4 Packet
    auto packet = static_unique_ptr_cast<net::PacketIP4>(std::move(pckt));

    // Stat increment packets received
    packets_rx_++;

    ip_header* hdr = &packet->ip_header();

    // Drop if my ip address doesn't match destination ip address or broadcast
    if(UNLIKELY(hdr->daddr != local_ip() and
                (hdr->daddr | stack_.netmask()) != ADDR_BCAST)) {

      if (forward_packet_)
        forward_packet_(stack_, static_unique_ptr_cast<IP_packet>(std::move(pckt)));
      else
        packets_dropped_++;

      return;
    }

    debug2("\t Source IP: %s Dest.IP: %s\n",
           hdr->saddr.str().c_str(), hdr->daddr.str().c_str());

    switch(hdr->protocol){
    case IP4_ICMP:
      debug2("\t Type: ICMP\n");
      icmp_handler_(std::move(packet));
      break;
    case IP4_UDP:
      debug2("\t Type: UDP\n");
      udp_handler_(std::move(packet));
      break;
    case IP4_TCP:
      tcp_handler_(std::move(packet));
      debug2("\t Type: TCP\n");
      break;
    default:
      debug("\t Type: UNKNOWN %i\n", hdr->protocol);
      break;
    }
  }

  uint16_t IP4::checksum(ip4::Header* hdr) {
    return net::checksum(reinterpret_cast<uint16_t*>(hdr), sizeof(ip4::Header));
  }

  void IP4::transmit(Packet_ptr pckt) {
    assert(pckt->size() > sizeof(IP4::full_header));

    auto ip4_pckt = static_unique_ptr_cast<PacketIP4>(std::move(pckt));
    ip4_pckt->make_flight_ready();

    IP4::ip_header& hdr = ip4_pckt->ip_header();
    // Create local and target subnets
    addr target = hdr.daddr        & stack_.netmask();
    addr local  = stack_.ip_addr() & stack_.netmask();

    // Compare subnets to know where to send packet
    ip4_pckt->next_hop(target == local ? hdr.daddr : stack_.gateway());

    debug("<IP4 TOP> Next hop for %s, (netmask %s, local IP: %s, gateway: %s) == %s\n",
          hdr.daddr.str().c_str(),
          stack_.netmask().str().c_str(),
          stack_.ip_addr().str().c_str(),
          stack_.router().str().c_str(),
          target == local ? "DIRECT" : "GATEWAY");

    debug("<IP4 transmit> my ip: %s, Next hop: %s, Packet size: %i IP4-size: %i\n",
          stack_.ip_addr().str().c_str(),
          pckt->next_hop().str().c_str(),
          pckt->size(),
          ip4_pckt->ip_segment_size()
          );

    // Stat increment packets transmitted
    packets_tx_++;

    linklayer_out_(std::move(ip4_pckt));
  }

  // Empty handler for delegates initialization
  void ignore_ip4_up(Packet_ptr) {
    debug("<IP4> Empty handler. Ignoring.\n");
  }

  void ignore_ip4_down(Packet_ptr) {
    debug("<IP4->Link layer> No handler - DROP!\n");
  }

} //< namespace net
