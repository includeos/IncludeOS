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

#include <stdint.h>
#include "ip6.hpp"

namespace net
{
  class NDP
  {
  public:
    struct router_sol
    {
      uint8_t  type;
      uint8_t  code;
      uint16_t checksum;
      uint32_t reserved;
      uint8_t  options[0];
      
    } __attribute__((packed));
    
    struct neighbor_sol
    {
      uint8_t   type;
      uint8_t   code;
      uint16_t  checksum;
      uint32_t  reserved;
      IP6::addr target;
      uint8_t   options[0];
      
    } __attribute__((packed));
    
    struct neighbor_adv
    {
      uint8_t   type;
      uint8_t   code;
      uint16_t  checksum;
      uint32_t  rso_reserved;
      IP6::addr target;
      uint8_t   options[0];
      
      static const uint32_t R = 0x1;
      static const uint32_t S = 0x2;
      static const uint32_t O = 0x4;
      
      void set_rso(uint32_t rso_flags)
      {
        rso_reserved = htonl(rso_flags & 7);
      }
      
    } __attribute__((packed));
    
    
    
  };
}
