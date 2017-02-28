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

// #define DEBUG // Allow debugging
// #define DEBUG2

#include <net/util.hpp>
#include <net/ethernet/ethernet.hpp>
#include <statman>

#ifdef ntohs
#undef ntohs
#endif

namespace net {

  static void ignore(net::Packet_ptr) noexcept {
    debug("<Ethernet upstream> Ignoring data (no real upstream)\n");
  }

  Ethernet::Ethernet(downstream physical_downstream, const addr& mac) noexcept
  : mac_(mac),
    packets_rx_{Statman::get().create(Stat::UINT64, ".ethernet.packets_rx").get_uint64()},
    packets_tx_{Statman::get().create(Stat::UINT64, ".ethernet.packets_tx").get_uint64()},
    packets_dropped_{Statman::get().create(Stat::UINT32, ".ethernet.packets_dropped").get_uint32()},
    ip4_upstream_{ignore},
    ip6_upstream_{ignore},
    arp_upstream_{ignore},
    physical_downstream_(physical_downstream)
{
}

  void Ethernet::transmit(net::Packet_ptr pckt, addr dest, Ethertype type)
  {

    debug("<Ethernet OUT> Transmitting %i b, from %s -> %s. Type: 0x%x\n",
          pckt->size(), mac_.str().c_str(), dest.str().c_str(), type);

    Expects(dest.major or dest.minor);
    Expects((size_t)(pckt->layer_begin() - pckt->buf()) >= sizeof(header));

    // Populate ethernet header for each packet in the (potential) chain
    // NOTE: It's assumed that chained packets are for the same destination
    auto* next = pckt.get();

    do {
      // Demote to ethernet frame
      next->increment_layer_begin(- sizeof(header));

      auto& hdr = *reinterpret_cast<header*>(next->layer_begin());

      // Add source address
      hdr.set_src(mac_);
      hdr.set_dest(dest);
      hdr.set_type(type);
      debug(" \t <Eth unchain> Transmitting %i b, from %s -> %s. Type: 0x%x\n",
            next->size(), mac_.str().c_str(), hdr.dest().str().c_str(), hdr.type());

      // Stat increment packets transmitted
      packets_tx_++;

      next = next->tail();

    } while (next);

    physical_downstream_(std::move(pckt));
  }

  void Ethernet::receive(Packet_ptr pckt) {
    Expects(pckt->size() > 0);

    header* eth = reinterpret_cast<header*>(pckt->layer_begin());

    debug("<Ethernet IN> %s => %s , Eth.type: 0x%x ",
          eth->src().str().c_str(), eth->dest().str().c_str(), eth->type());

    // Stat increment packets received
    packets_rx_++;

    bool dropped = false;

    switch(eth->type()) {
    case Ethertype::IP4:
      debug2("IPv4 packet\n");
      pckt->increment_layer_begin(sizeof(header));
      ip4_upstream_(std::move(pckt));
      break;

    case Ethertype::IP6:
      debug2("IPv6 packet\n");
      pckt->increment_layer_begin(sizeof(header));
      ip6_upstream_(std::move(pckt));
      break;

    case Ethertype::ARP:
      debug2("ARP packet\n");
      pckt->increment_layer_begin(sizeof(header));
      arp_upstream_(std::move(pckt));
      break;

    case Ethertype::WOL:
      dropped = true;
      debug2("Wake-on-LAN packet\n");
      break;

    case Ethertype::VLAN:
      dropped = true;
      debug("VLAN tagged frame (not yet supported)");
      break;

    default:
      dropped = true;
      uint16_t length_field = net::ntohs(static_cast<uint16_t>(eth->type()));
      // This might be 802.3 LLC traffic
      if (length_field > 1500) {
        debug2("<Ethernet> UNKNOWN ethertype 0x%x\n", ntohs(eth->type()));
      }else {
        debug2("IEEE802.3 Length field: 0x%x\n", ntohs(eth->type()));
      }
      break;
    }

    if(dropped)
      packets_dropped_++;
  }

} // namespace net
