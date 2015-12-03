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

#include "../ip4.hpp"
#include "../packet.hpp"
#include "../util.hpp"
#include <cassert>

namespace net
{
  class PacketIP4 : public Packet, // might work as upcast:
                    public std::enable_shared_from_this<PacketIP4>
  {
  public:
    
    static const int DEFAULT_TTL = 64;
    
    const IP4::addr& src() const
    {
      return ip4_header().saddr;
    }
    void set_src(const IP4::addr& addr)
    {
      ip4_header().saddr = addr;
    }
    
    const IP4::addr& dst() const
    {
      return ip4_header().daddr;
    }
    void set_dst(const IP4::addr& addr)
    {
      ip4_header().daddr = addr;
    }
    
    void set_protocol(IP4::proto p){
      ip4_header().protocol = p;
    }
    
    uint8_t protocol() const
    {
      return ip4_header().protocol;
    }
    
    uint16_t ip4_segment_size()
    {
      return ntohs(ip4_header().tot_len);
    }
        
    
    /** Last modifications before transmission */
    void make_flight_ready(){
      assert( ip4_header().protocol );
      set_segment_length();
      set_ip4_checksum();
    }
    
    void init() {
      
      ip4_header().version_ihl = 0x45;
      ip4_header().tos = 0;
      ip4_header().id  = 0;
      ip4_header().frag_off_flags = 0;
      ip4_header().ttl = DEFAULT_TTL;
      
    }
    
  private:
        
    const IP4::ip_header& ip4_header() const
    {
      return ((IP4::full_header*) buffer())->ip_hdr;
    }
    
    IP4::ip_header& ip4_header()
    {
      return ((IP4::full_header*) buffer())->ip_hdr;
    }

    /** Set IP4 header lenght, inferred from packet size and linklayer header size */
    void set_segment_length()
    {
      ip4_header().tot_len = htons(size() - sizeof(LinkLayer::header));
    }
    
    void set_ip4_checksum()
    {
      auto& hdr = ip4_header();
      hdr.check = 0;
      hdr.check = net::checksum(&hdr, sizeof(IP4::ip_header));
    }
    
  };
}
