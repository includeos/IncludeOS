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
  class PacketArp : public Packet,
                    public std::enable_shared_from_this<PacketArp>
  {
  public:
    Arp::header& header() const
    {
      return *(Arp::header*) buffer();
    }
    
    static const size_t headers_size = sizeof(Arp::header);
    
    /** initializes to a default, empty Arp packet, given
        a valid MTU-sized buffer */
    void init(Ethernet::addr local_mac, IP4::addr local_ip)
    {            
      auto& hdr = header();
      hdr.ethhdr.type = Ethernet::ETH_ARP;
      hdr.htype = Arp::H_htype_eth;
      hdr.ptype = Arp::H_ptype_ip4;
      hdr.hlen_plen = Arp::H_hlen_plen;
      
      hdr.dipaddr = next_hop();
      hdr.sipaddr = local_ip;
      hdr.shwaddr = local_mac;
    }
    
    void set_dest_mac(Ethernet::addr mac) {
      header().dhwaddr = mac;
      header().ethhdr.dest = mac; 
    } 
    
    void set_opcode(Arp::Opcode op) {
      header().opcode = op;
    }
    
    void set_dest_ip(IP4::addr ip) {
      header().dipaddr = ip;
    }
    
    IP4::addr source_ip() const {
      return header().sipaddr;
    }
    
    IP4::addr dest_ip() const {
      return header().dipaddr;
    }
    
    Ethernet::addr source_mac() const {
      return header().ethhdr.src;
    };
    
    Ethernet::addr dest_mac() const {
      return header().ethhdr.dest;
    };
    
    
  };
}

#endif
