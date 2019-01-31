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

#pragma once
#ifndef NET_IP4_PACKET_ARP
#define NET_IP4_PACKET_ARP

#include "arp.hpp"
#include <net/packet.hpp>

namespace net
{
  class PacketArp : public Packet
  {

  public:

    Arp::header& header() const
    {
      return *reinterpret_cast<Arp::header*>(layer_begin());
    }

    static const size_t headers_size = sizeof(Arp::header);

    /** initializes to a default, empty Arp packet, given
        a valid MTU-sized buffer */
    void init(MAC::Addr local_mac, ip4::Addr local_ip, ip4::Addr dest_ip)
    {

      auto& hdr = header();
      hdr.htype = Arp::H_htype_eth;
      hdr.ptype = Arp::H_ptype_ip4;
      hdr.hlen_plen = Arp::H_hlen_plen;

      hdr.dipaddr = dest_ip;
      hdr.sipaddr = local_ip;
      hdr.shwaddr = local_mac;

      // We've effectively added data to the packet
      increment_data_end(sizeof(Arp::header));
    }

    void set_dest_mac(MAC::Addr mac) {
      header().dhwaddr = mac;
    }

    void set_opcode(Arp::Opcode op) {
      header().opcode = op;
    }

    void set_dest_ip(ip4::Addr ip) {
      header().dipaddr = ip;
    }

    ip4::Addr source_ip() const {
      return header().sipaddr;
    }

    ip4::Addr dest_ip() const {
      return header().dipaddr;
    }

    MAC::Addr source_mac() const {
      return header().shwaddr;
    };

    MAC::Addr dest_mac() const {
      return header().dhwaddr;
    };


  };
}

#endif
