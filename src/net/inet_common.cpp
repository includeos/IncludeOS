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

#include <stdlib.h>

#include <os>
#include <net/util.hpp>
#include <net/inet_common.hpp>

namespace net {

  // Should be pretty much like the example in RFC 1071,
  // but using a uinon for readability
  uint16_t checksum(void* data, size_t len) noexcept {

    uint16_t* buf = reinterpret_cast<uint16_t*>(data);

    union sum {
      uint32_t whole;    
      uint16_t part[2];
    } sum32 {0};
  
    // Iterate in short int steps.
    for (uint16_t* i = buf; i < (buf + len / 2); ++i)
      sum32.whole += *i;
  
    // odd-length case
    if (len & 1) {  
      sum32.whole += reinterpret_cast<uint8_t*>(buf)[len - 1];
    }

    return ~(sum32.part[0] + sum32.part[1]);
  }

} //< namespace net
