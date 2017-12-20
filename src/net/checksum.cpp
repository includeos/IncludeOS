// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2017 Oslo and Akershus University College of Applied Sciences
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

#include <net/checksum.hpp>
#include <immintrin.h>
#include <x86intrin.h>
#include <cassert>

namespace net {

uint16_t checksum(uint32_t tsum, const void* data, size_t length) noexcept
{
  const char* buffer = (const char*) data;
  int64_t sum = tsum;

  // VEX-align buffer
  while (((uintptr_t) buffer & 15) && length >= 4) {
    sum += *(uint32_t*) buffer;
    length -= 4; buffer += 4;
  }

#ifdef __AVX2__
  // run 4 32-bit adds in parallell
  union vec4 {
    __m256i  mm;
    int64_t  epi64[4];
  };
  vec4 vsum; vsum.mm = _mm256_set1_epi32(0);
  while (length >= 64)
  {
    __m128i tmp1 = _mm_load_si128((__m128i*) (buffer + 0));
    __m128i tmp2 = _mm_load_si128((__m128i*) (buffer + 16));
    __m128i tmp3 = _mm_load_si128((__m128i*) (buffer + 32));
    __m128i tmp4 = _mm_load_si128((__m128i*) (buffer + 48));

    // unpack 16x 32-bit values
    __m256i epi64_1 = _mm256_cvtepu32_epi64(tmp1);
    __m256i epi64_2 = _mm256_cvtepu32_epi64(tmp2);
    __m256i epi64_3 = _mm256_cvtepu32_epi64(tmp3);
    __m256i epi64_4 = _mm256_cvtepu32_epi64(tmp4);

    // sum 16x 32-bit values
    vsum.mm = _mm256_add_epi64(vsum.mm, epi64_1);
    vsum.mm = _mm256_add_epi64(vsum.mm, epi64_2);
    vsum.mm = _mm256_add_epi64(vsum.mm, epi64_3);
    vsum.mm = _mm256_add_epi64(vsum.mm, epi64_4);

    length -= 64; buffer += 64;
  }
  // horizontal add
  sum += vsum.epi64[0];
  sum += vsum.epi64[1];
  sum += vsum.epi64[2];
  sum += vsum.epi64[3];
#endif

  // unrolled 8 32-bit adds
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

// Taken from https://tools.ietf.org/html/rfc3022#page-9
void checksum_adjust(uint8_t* chksum, const void* odata,
   int olen, const void* ndata, int nlen)
{
  assert(olen % 2 == 0 and nlen % 2 == 0);

  const auto* optr = reinterpret_cast<const uint8_t*>(odata);
  const auto* nptr = reinterpret_cast<const uint8_t*>(ndata);

  int32_t x, o32, n32;
  x=chksum[0]*256+chksum[1];
  x=~x & 0xFFFF;
  while (olen)
  {
    o32=optr[0]*256+optr[1]; optr+=2;
    x-=o32 & 0xffff;
    if (x<=0) { x--; x&=0xffff; }
    olen-=2;
  }
  while (nlen)
  {
    n32=nptr[0]*256+nptr[1]; nptr+=2;
    x+=n32 & 0xffff;
    if (x & 0x10000) { x++; x&=0xffff; }
    nlen-=2;
  }
  x=~x & 0xFFFF;
  chksum[0]=x/256; chksum[1]=x & 0xff;
}

} //< namespace net
