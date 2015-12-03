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
#include "ip6.hpp"

namespace net
{
  
  class PacketIP6 : public Packet
  {
  public:
    IP6::header& ip6_header()
    {
      return ((IP6::full_header*) buffer())->ip6_hdr;
    }
    const IP6::header& ip6_header() const
    {
      return ((IP6::full_header*) buffer())->ip6_hdr;
    }
    Ethernet::header& eth_header()
    {
      return *(Ethernet::header*) buffer();
    }
    
    const IP6::addr& src() const
    {
      return ip6_header().src;
    }
    void set_src(const IP6::addr& src)
    {
      ip6_header().src = src;
    }
    
    const IP6::addr& dst() const
    {
      return ip6_header().dst;
    }
    void set_dst(const IP6::addr& dst)
    {
      ip6_header().dst = dst;
    }
    
    uint8_t hoplimit() const
    {
      return ip6_header().hoplimit();
    }
    void set_hoplimit(uint8_t limit)
    {
      ip6_header().set_hoplimit(limit);
    }
    
    // returns the protocol type of the next header
    uint8_t next() const
    {
      return ip6_header().next();
    }
    void set_next(uint8_t next)
    {
      ip6_header().set_next(next);
    }
    
  }; // PacketIP6
  
}
