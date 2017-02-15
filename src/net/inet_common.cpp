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

#include <net/inet_common.hpp>

namespace net {

uint16_t checksum(uint32_t sum, const void* data, size_t length) noexcept
{
  const char* buffer = (const char*) data;
  
  while (length >= 4)
  {
    auto v = *(uint32_t*) buffer;
    sum += v;
    if (sum < v) sum++;
    length -= 4; buffer += 4;
  }
  if (length & 2)
  {
    auto v = *(uint16_t*) buffer;
    sum += v;
    if (sum < v) sum++;
    buffer += 2;
  }
  if (length & 1)
  {
    auto v = *(uint8_t*) buffer;
    sum += v;
    if (sum < v) sum++;
  }
  // Fold to 16-bit
  uint16_t a = sum & 0xffff;
  uint16_t b = sum >> 16;
  a += b;
  if (a < b) a++;
  return ~a;
}

} //< namespace net
