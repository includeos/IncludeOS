// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2018 IncludeOS AS, Oslo, Norway
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

#include <net/packet.hpp>
#include <net/ethernet/ethernet.hpp>

namespace net {

  static net::Packet* last_packet = nullptr;
  static uint8_t* layer_begin = nullptr;

  void set_last_packet(net::Packet* ptr)
  {
    Expects(ptr != nullptr);
    last_packet = ptr;
    layer_begin = ptr->layer_begin();
  }

  void print_last_packet()
  {
    if(last_packet == nullptr)
      return;

    auto* pkt = last_packet;
    fprintf(stderr, "*** Last packet:\n");
    fprintf(stderr, "Buffer: Begin: %p End: %p Size: %i\n",
      pkt->buf(), pkt->buffer_end(), pkt->bufsize());
    const size_t offset = pkt->layer_begin() - layer_begin;
    fprintf(stderr, "Layer: Recorded: %p Current: %p (%lub offset)\n",
      layer_begin, pkt->layer_begin(), offset);
    fprintf(stderr, "Size: %i ", pkt->size());
    fprintf(stderr, "Capacity: %i ", pkt->capacity());
    fprintf(stderr, "Data end: %p \n", pkt->data_end());

    // assume ethernet
    auto* layer = layer_begin;
    Ethernet::header* eth = reinterpret_cast<Ethernet::header*>(layer);
    fprintf(stderr, "Ethernet type: 0x%hx ", eth->type());
    int print_len = sizeof(Ethernet::header);
    switch(eth->type())
    {
      case Ethertype::IP4:
      fprintf(stderr, "IPv4\n");
      print_len += 20;
      break;
    case Ethertype::IP6:
      fprintf(stderr, "IPv6\n");
      print_len += 40;
      break;
    case Ethertype::ARP:
      fprintf(stderr, "ARP\n");
      print_len += 28;
      break;
    case Ethertype::VLAN:
      fprintf(stderr, "VLAN\n");
      print_len += 4;
      break;
    default:
      fprintf(stderr, "Unknown\n");
      print_len = 0;
    }

    // ignore the above, just hope we can write the full content of the packet
    print_len = offset + pkt->size();

    fprintf(stderr, "Payload %i bytes from recorded layer begin (%p):", print_len, layer_begin);
    for(int i = 0; i < print_len; i++) {
      if(i % 80 == 0) fprintf(stderr, "\n"); // break every 80th char
      fprintf(stderr, "%02x", *(layer_begin + i));
    }
    fprintf(stderr, "\n");
  }
}
