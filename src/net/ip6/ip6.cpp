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

//#define DEBUG
#include <net/ip6/ip6.hpp>
#include <net/ip6/packet_ip6.hpp>

#include <assert.h>

namespace net
{
  const IP6::addr IP6::addr::node_all_nodes(0xFF01, 0, 0, 0, 0, 0, 0, 1);
  const IP6::addr IP6::addr::node_all_routers(0xFF01, 0, 0, 0, 0, 0, 0, 2);
  const IP6::addr IP6::addr::node_mDNSv6(0xFF01, 0, 0, 0, 0, 0, 0, 0xFB);

  const IP6::addr IP6::addr::link_unspecified(0, 0, 0, 0, 0, 0, 0, 0);

  const IP6::addr IP6::addr::link_all_nodes(0xFF02, 0, 0, 0, 0, 0, 0, 1);
  const IP6::addr IP6::addr::link_all_routers(0xFF02, 0, 0, 0, 0, 0, 0, 2);
  const IP6::addr IP6::addr::link_mDNSv6(0xFF02, 0, 0, 0, 0, 0, 0, 0xFB);

  const IP6::addr IP6::addr::link_dhcp_servers(0xFF02, 0, 0, 0, 0, 0, 0x01, 0x02);
  const IP6::addr IP6::addr::site_dhcp_servers(0xFF05, 0, 0, 0, 0, 0, 0x01, 0x03);

  IP6::IP6(const IP6::addr& lo)
    : local(lo)
  {
    assert(sizeof(addr)   == 16);
    assert(sizeof(header) == 40);
    assert(sizeof(options_header) == 8);
  }

  uint8_t IP6::parse6(uint8_t*& reader, uint8_t next)
  {
    switch (next)
      {
      case PROTO_HOPOPT:
      case PROTO_OPTSv6:
        {
          debug(">>> IPv6 options header %s", protocol_name(next).c_str());

          options_header& opts = *(options_header*) reader;
          reader += opts.size();

          debug("OPTSv6 size: %d\n", opts.size());
          debug("OPTSv6 ext size: %d\n", opts.extended());

          next = opts.next();
          debug("OPTSv6 next: %s\n", protocol_name(next).c_str());
        } break;
      case PROTO_ICMPv6:
        break;
      case PROTO_UDP:
        break;

      default:
        debug("Not parsing: %s\n", protocol_name(next).c_str());
      }

    return next;
  }

  void IP6::bottom(Packet_ptr pckt)
  {
    debug(">>> IPv6 packet:");



    uint8_t* reader = pckt->buffer();
    full_header& full = *(full_header*) reader;
    reader += sizeof(full_header);

    header& hdr = full.ip6_hdr;
    /*
      debug("IPv6 v: %i \t class: %i  size: %i \n",
      (int) hdr.version(), (int) hdr.tclass(), hdr.size() )";
    */
    uint8_t next = hdr.next();

    while (next != PROTO_NoNext)
      {
        auto it = proto_handlers.find(next);
        if (it != proto_handlers.end())
          {
            // forward packet to handler
            pckt->set_payload(reader);
            it->second(pckt);
          }
        else
          // just print information
          next = parse6(reader, next);
      }

  };

  static const std::string lut = "0123456789abcdef";

  std::string IP6::addr::str() const
  {
    std::string ret(40, 0);
    int counter = 0;

    const uint8_t* octet = i8;

    for (int i = 0; i < 16; i++)
      {
        ret[counter++] = lut[(octet[i] & 0xF0) >> 4];
        ret[counter++] = lut[(octet[i] & 0x0F) >> 0];
        if (i & 1)
          ret[counter++] = ':';
      }
    ret.resize(counter-1);
    return ret;
  }

  void IP6::transmit(std::shared_ptr<PacketIP6>& ip6_packet)
  {
    auto packet = *reinterpret_cast<std::shared_ptr<Packet>*> (&ip6_packet);

    //debug("<IP6 OUT> Transmitting %li b, from %s -> %s\n",
    //       pckt->len(), hdr.src.str().c_str(), hdr.dst.str().c_str());

    _linklayer_out(packet);
  }

  std::shared_ptr<PacketIP6> IP6::create(uint8_t proto,
                                         Ethernet::addr ether_dest, const IP6::addr& ip6_dest)
  {
    // arbitrarily big buffer
    uint8_t* data = new uint8_t[1580];

    // @WARNING: Initializing packet as "full", i.e. size == capacity
    Packet* packet = (Packet*) data;
    new (packet) Packet(sizeof(data), 0);

    IP6::full_header& full = *(IP6::full_header*) packet->buffer();
    // people dont think that it be, but it do
    full.eth_hdr.type = Ethernet::ETH_IP6;
    full.eth_hdr.dest = ether_dest;

    IP6::header& hdr = full.ip6_hdr;

    // set IPv6 packet parameters
    hdr.src = addr::link_unspecified;
    hdr.dst = ip6_dest;
    // default header frame
    hdr.init_scan0();
    // protocol for next header
    hdr.set_next(proto);
    // default hoplimit
    hdr.set_hoplimit(64);

    // common offset of payload
    packet->set_payload(packet->buffer() + sizeof(IP6::full_header));

    auto ip6_packet =
      std::shared_ptr<PacketIP6> ((PacketIP6*) packet,
          [] (void* ptr) { delete[] (uint8_t*) ptr; });
    // now, free to use :)
    return ip6_packet;
  }

}
