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

//#define VLAN_DEBUG 1
#ifdef VLAN_DEBUG
#define PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define PRINT(fmt, ...) /* fmt */
#endif

#include <net/ethernet/ethernet_8021q.hpp>

namespace net {

Ethernet_8021Q::Ethernet_8021Q(downstream phys_down,
                               const addr& mac, const int id) noexcept
  : Ethernet(phys_down, mac),
    id_{id}
{}

void Ethernet_8021Q::receive(Packet_ptr pkt)
{
  auto& vlan = *reinterpret_cast<ethernet::VLAN_header*>(pkt->layer_begin());

  PRINT("<802.1Q IN> %#x (id %d) - ", vlan.tpid, vlan.vid());

  switch(vlan.type) {
  case Ethertype::IP4:
    PRINT("IPv4 packet\n");
    pkt->increment_layer_begin(header_size());
    ip4_upstream_(std::move(pkt), vlan.dest == MAC::BROADCAST);
    break;

  case Ethertype::IP6:
    PRINT("IPv6 packet\n");
    pkt->increment_layer_begin(header_size());
    ip6_upstream_(std::move(pkt), vlan.dest == MAC::BROADCAST);
    break;

  case Ethertype::ARP:
    PRINT("ARP packet\n");
    pkt->increment_layer_begin(header_size());
    arp_upstream_(std::move(pkt));
    break;

  case Ethertype::WOL:
    PRINT("Wake-on-LAN packet\n");
    break;

  case Ethertype::VLAN:
    PRINT("VLAN frame\n");
    assert(false && "Do not support multiple VLAN atm.. O.o");
    break;

  default:
    uint16_t type = ntohs(static_cast<uint16_t>(vlan.type));

    // Trailer negotiation and encapsulation RFC 893 and 1122
    if (UNLIKELY(type == ntohs(static_cast<uint16_t>(Ethertype::TRAILER_NEGO)) or
      (type >= ntohs(static_cast<uint16_t>(Ethertype::TRAILER_FIRST)) and
        type <= ntohs(static_cast<uint16_t>(Ethertype::TRAILER_LAST))))) {
      printf("Trailer packet\n");
      break;
    }

    // This might be 802.3 LLC traffic
    if (type > 1500) {
      PRINT("<802.1Q> UNKNOWN ethertype 0x%hx\n", vlan.type);
    } else {
      PRINT("IEEE802.3 Length field: 0x%hx\n", vlan.type);
    }
    break;
  }
}

void Ethernet_8021Q::transmit(Packet_ptr pkt, addr dest, Ethertype type)
{
  uint16_t t = ntohs(static_cast<uint16_t>(type));
  // Trailer negotiation and encapsulation RFC 893 and 1122
  if (UNLIKELY(t == ntohs(static_cast<uint16_t>(Ethertype::TRAILER_NEGO)) or
    (t >= ntohs(static_cast<uint16_t>(Ethertype::TRAILER_FIRST)) and
      t <= ntohs(static_cast<uint16_t>(Ethertype::TRAILER_LAST))))) {
    PRINT("<802.1Q OUT> Ethernet type Trailer is not supported. Packet is not transmitted\n");
    return;
  }

  // make sure packet is minimum ethernet frame size
  //if (pckt->size() < 68) pckt->set_data_end(68);

  PRINT("<802.1Q OUT> Transmitting %i b, from %s -> %s. Type: 0x%hx ID: %d\n",
        pkt->size(), mac_.str().c_str(), dest.str().c_str(), type, id_);

  // Populate ethernet header for each packet in the (potential) chain
  // NOTE: It's assumed that chained packets are for the same destination
  auto* next = pkt.get();

  do {
    // Demote to VLAN frame
    next->increment_layer_begin(- (int)sizeof(ethernet::VLAN_header));

    auto& hdr = *reinterpret_cast<ethernet::VLAN_header*>(next->layer_begin());

    // Add source address
    hdr.src = mac_;
    hdr.dest = dest;

    hdr.type = type;
    hdr.set_vid(id_);
    hdr.tpid = static_cast<uint16_t>(Ethertype::VLAN);
    PRINT(" \t <802.1 unchain> Transmitting %i b, from %s -> %s. Type: 0x%hx ID: %d\n",
          next->size(), hdr.src.str().c_str(), hdr.dest.str().c_str(), hdr.type, hdr.vid());

    // Stat increment packets transmitted
    packets_tx_++;

    next = next->tail();

  } while (next);

  physical_downstream_(std::move(pkt));
}

} // < namespace net
