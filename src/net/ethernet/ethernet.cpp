// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2017 Oslo and Akershus University College of Applied Sciences
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

//#define ETH_DEBUG 1
#ifdef ETH_DEBUG
#define PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define PRINT(fmt, ...) /* fmt */
#endif

#include <net/util.hpp>
#include <net/ethernet/ethernet.hpp>
#include <statman>

#ifdef ntohs
#undef ntohs
#endif

namespace net {

  static void ignore(net::Packet_ptr) noexcept {
    PRINT("<Ethernet upstream> Ignoring data (no real upstream)\n");
  }
  static void ignore_ip(net::Packet_ptr, const bool) noexcept {
    debug("<Ethernet upstream_ip> Ignoring data (no real upstream)\n");
  }
  static int eth_name_idx = 0;

  Ethernet::Ethernet(
        downstream physical_downstream,
        const addr& mac) noexcept
  : mac_(mac),
    ethernet_idx(eth_name_idx++),
    packets_rx_{Statman::get().create(Stat::UINT64,
                link_name() + ".ethernet.packets_rx").get_uint64()},
    packets_tx_{Statman::get().create(Stat::UINT64,
                link_name() + ".ethernet.packets_tx").get_uint64()},
    packets_dropped_{Statman::get().create(Stat::UINT32,
                link_name() + ".ethernet.packets_dropped").get_uint32()},
    trailer_packets_dropped_{Statman::get().create(Stat::UINT32,
                link_name() + ".ethernet.trailer_packets_dropped").get_uint32()},
    ip4_upstream_{ignore_ip},
    ip6_upstream_{ignore_ip},
    arp_upstream_{ignore},
    physical_downstream_(physical_downstream)
  {}

  void Ethernet::transmit(net::Packet_ptr pckt, addr dest, Ethertype type)
  {
    uint16_t t = net::ntohs(static_cast<uint16_t>(type));
    // Trailer negotiation and encapsulation RFC 893 and 1122
    if (UNLIKELY(t == net::ntohs(static_cast<uint16_t>(Ethertype::TRAILER_NEGO)) or
      (t >= net::ntohs(static_cast<uint16_t>(Ethertype::TRAILER_FIRST)) and
        t <= net::ntohs(static_cast<uint16_t>(Ethertype::TRAILER_LAST))))) {
      PRINT("<Ethernet OUT> Ethernet type Trailer is not supported. Packet is not transmitted\n");
      return;
    }

    // make sure packet is minimum ethernet frame size
    //if (pckt->size() < 68) pckt->set_data_end(68);

    PRINT("<Ethernet OUT> Transmitting %i b, from %s -> %s. Type: 0x%hx\n",
          pckt->size(), mac_.str().c_str(), dest.str().c_str(), type);
#ifndef ARP_PASSTHROUGH
    Expects(dest.major or dest.minor);
#endif

    // Populate ethernet header for each packet in the (potential) chain
    // NOTE: It's assumed that chained packets are for the same destination
    auto* next = pckt.get();

    do {
      // Demote to ethernet frame
      next->increment_layer_begin(- (int)sizeof(header));

      auto& hdr = *reinterpret_cast<header*>(next->layer_begin());

      // Add source address
      hdr.set_src(mac_);
      hdr.set_dest(dest);
      hdr.set_type(type);
      PRINT(" \t <Eth unchain> Transmitting %i b, from %s -> %s. Type: 0x%hx\n",
            next->size(), mac_.str().c_str(), hdr.dest().str().c_str(), hdr.type());

      // Stat increment packets transmitted
      packets_tx_++;

      next = next->tail();

    } while (next);

    physical_downstream_(std::move(pckt));
  }

#ifdef ARP_PASSTHROUGH
  MAC::Addr linux_tap_device;
#endif
  void Ethernet::receive(Packet_ptr pckt) {
    Expects(pckt->size() > 0);

    header* eth = reinterpret_cast<header*>(pckt->layer_begin());

    PRINT("<Ethernet IN> %s => %s , Eth.type: 0x%hx ",
          eth->src().str().c_str(), eth->dest().str().c_str(), eth->type());

#ifdef ARP_PASSTHROUGH
    linux_tap_device = eth->src();
#endif

    // Stat increment packets received
    packets_rx_++;

    switch(eth->type()) {
    case Ethertype::IP4:
      PRINT("IPv4 packet\n");
      pckt->increment_layer_begin(sizeof(header));
      ip4_upstream_(std::move(pckt), eth->dest() == MAC::BROADCAST);
      break;

    case Ethertype::IP6:
      PRINT("IPv6 packet\n");
      pckt->increment_layer_begin(sizeof(header));
      ip6_upstream_(std::move(pckt), eth->dest() == MAC::BROADCAST);
      break;

    case Ethertype::ARP:
      PRINT("ARP packet\n");
      pckt->increment_layer_begin(sizeof(header));
      arp_upstream_(std::move(pckt));
      break;

    case Ethertype::WOL:
      packets_dropped_++;
      PRINT("Wake-on-LAN packet\n");
      break;

    case Ethertype::VLAN:
      PRINT("VLAN frame\n");
      if(not vlan_upstream_)
        packets_dropped_++;
      else {
        vlan_upstream_(std::move(pckt));
      }
      break;

    default:
      uint16_t type = net::ntohs(static_cast<uint16_t>(eth->type()));
      packets_dropped_++;

      // Trailer negotiation and encapsulation RFC 893 and 1122
      if (UNLIKELY(type == net::ntohs(static_cast<uint16_t>(Ethertype::TRAILER_NEGO)) or
        (type >= net::ntohs(static_cast<uint16_t>(Ethertype::TRAILER_FIRST)) and
          type <= net::ntohs(static_cast<uint16_t>(Ethertype::TRAILER_LAST))))) {
        trailer_packets_dropped_++;
        PRINT("Trailer packet\n");
        break;
      }

      // This might be 802.3 LLC traffic
      if (type > 1500) {
        PRINT("<Ethernet> UNKNOWN ethertype 0x%hx\n", eth->type());
      } else {
        PRINT("IEEE802.3 Length field: 0x%hx\n", eth->type());
      }

      break;
    }

  }

} // namespace net
