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

// #define DEBUG // Allow debugging
// #define DEBUG2 // Allow debug lvl 2

#include <net/ip4/ip4.hpp>
#include <net/ip4/packet_ip4.hpp>
#include <net/packet.hpp>
#include <statman>

namespace net {

  const IP4::addr IP4::ADDR_ANY(0);
  const IP4::addr IP4::ADDR_BCAST(0xff,0xff,0xff,0xff);

  IP4::IP4(Stack& inet) noexcept :
  packets_rx_       {Statman::get().create(Stat::UINT64, inet.ifname() + ".ip4.packets_rx").get_uint64()},
  packets_tx_       {Statman::get().create(Stat::UINT64, inet.ifname() + ".ip4.packets_tx").get_uint64()},
  packets_dropped_  {Statman::get().create(Stat::UINT32, inet.ifname() + ".ip4.packets_dropped").get_uint32()},
  stack_            {inet}
 { }

  void IP4::receive(Packet_ptr pckt)
  {
    // Cast to IP4 Packet
    auto packet = static_unique_ptr_cast<net::PacketIP4>(std::move(pckt));

    // Stat increment packets received
    packets_rx_++;

    debug2("\t Source IP: %s Dest.IP: %s Type: 0x%x\n",
           packet->src().str().c_str(), 
           packet->dst().str().c_str(), 
           packet->protocol());

    // Drop if my ip address doesn't match destination ip address or broadcast
    if (UNLIKELY(packet->dst() != local_ip()
            and (packet->dst() | stack_.netmask()) != ADDR_BCAST
            and local_ip() != ADDR_ANY)) {

      if (forward_packet_) {
        forward_packet_(stack_, std::move(packet));
        debug("Packet forwarded \n");
      } else {
        debug("Packet dropped \n");
        packets_dropped_++;
      }

      return;
    }

    switch(packet->protocol()){
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
      debug("\t Type: UNKNOWN %i\n", hdr->protocol);
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

    addr next_hop;
    // Keep IP when broadcasting to all
    if (ip4_pckt->dst() != IP4::ADDR_BCAST)
    {
      // Create local and target subnets
      addr target = ip4_pckt->dst()  & stack_.netmask();
      addr local  = stack_.ip_addr() & stack_.netmask();

      // Compare subnets to know where to send packet
      next_hop = target == local ? ip4_pckt->dst() : stack_.gateway();

      debug("<IP4 TOP> Next hop for %s, (netmask %s, local IP: %s, gateway: %s) == %s\n",
          ip4_pckt->dst().str().c_str(),
          stack_.netmask().str().c_str(),
          stack_.ip_addr().str().c_str(),
          stack_.gateway().str().c_str(),
          next_hop.str().c_str());
    }
    else {
      next_hop = IP4::ADDR_BCAST;
    }

    // Stat increment packets transmitted
    packets_tx_++;

    debug("<IP4> Transmitting packet, layer begin: buf + %i\n", ip4_pckt->layer_begin() - ip4_pckt->buf());

    linklayer_out_(std::move(ip4_pckt), next_hop);
  }


} //< namespace net
