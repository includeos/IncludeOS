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

uint16_t checksum(uint32_t tsum, const void* data, size_t length) noexcept
{
  const char* buffer = (const char*) data;
  int64_t sum = tsum;
  // unrolled 32 bytes at once
  while (length >= 32)
  {
    auto* v = (uint32_t*) buffer;
    sum += v[0];
    sum += v[1];
    sum += v[2];
    sum += v[3];
    sum += v[4];
    sum += v[5];
    sum += v[6];
    sum += v[7];
    length -= 32; buffer += 32;
  }
  while (length >= 4)
  {
    auto v = *(uint32_t*) buffer;
    sum += v;
    length -= 4; buffer += 4;
  }
  if (length & 2)
  {
    auto v = *(uint16_t*) buffer;
    sum += v;
    buffer += 2;
  }
  if (length & 1)
  {
    auto v = *(uint8_t*) buffer;
    sum += v;
  }
  // fold to 32-bit
  uint32_t a32 = sum & 0xffffffff;
  uint32_t b32 = sum >> 32;
  a32 += b32;
  if (a32 < b32) a32++;
  // fold again to 16-bit
  uint16_t a16 = a32 & 0xffff;
  uint16_t b16 = a32 >> 16;
  a16 += b16;
  if (a16 < b16) a16++;
  // return 2s complement
  return ~a16;
}

} //< namespace net
