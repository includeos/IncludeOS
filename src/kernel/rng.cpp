// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2016 Oslo and Akershus University College of Applied Sciences
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

#include <kernel/rng.hpp>
#include <kernel/cpuid.hpp>
#include <os.hpp>
#include <os.hpp>
#include <algorithm>
#include <cstring>
#include <smp>
#define SHAKE_128_RATE (1600-256)/8

struct alignas(SMP_ALIGN) rng_state
{
  uint64_t state[25];
  int64_t  reseed_counter = 0;
  int32_t  reseed_rounds  = 0;
  delegate<void(uint64_t*)> reseed_callback = nullptr;
};
static SMP::Array<rng_state> rng;
// every RESEED_RATE bytes entropy is refilled
static const int RESEED_RATE = 4096;

static inline uint64_t rotate_left(uint64_t input, size_t rot) {
  return (input << rot) | (input >> (64-rot));
}

static void keccak_1600_p(uint64_t A[25]) {
  static const uint64_t RC[24] = {
    0x0000000000000001, 0x0000000000008082, 0x800000000000808A,
    0x8000000080008000, 0x000000000000808B, 0x0000000080000001,
    0x8000000080008081, 0x8000000000008009, 0x000000000000008A,
    0x0000000000000088, 0x0000000080008009, 0x000000008000000A,
    0x000000008000808B, 0x800000000000008B, 0x8000000000008089,
    0x8000000000008003, 0x8000000000008002, 0x8000000000000080,
    0x000000000000800A, 0x800000008000000A, 0x8000000080008081,
    0x8000000000008080, 0x0000000080000001, 0x8000000080008008
  };

  for(size_t i = 0; i != 24; ++i)
  {
    const uint64_t C0 = A[0] ^ A[5] ^ A[10] ^ A[15] ^ A[20];
    const uint64_t C1 = A[1] ^ A[6] ^ A[11] ^ A[16] ^ A[21];
    const uint64_t C2 = A[2] ^ A[7] ^ A[12] ^ A[17] ^ A[22];
    const uint64_t C3 = A[3] ^ A[8] ^ A[13] ^ A[18] ^ A[23];
    const uint64_t C4 = A[4] ^ A[9] ^ A[14] ^ A[19] ^ A[24];

    const uint64_t D0 = rotate_left(C0, 1) ^ C3;
    const uint64_t D1 = rotate_left(C1, 1) ^ C4;
    const uint64_t D2 = rotate_left(C2, 1) ^ C0;
    const uint64_t D3 = rotate_left(C3, 1) ^ C1;
    const uint64_t D4 = rotate_left(C4, 1) ^ C2;

    const uint64_t B00 = A[ 0] ^ D1;
    const uint64_t B10 = rotate_left(A[ 1] ^ D2, 1);
    const uint64_t B20 = rotate_left(A[ 2] ^ D3, 62);
    const uint64_t B05 = rotate_left(A[ 3] ^ D4, 28);
    const uint64_t B15 = rotate_left(A[ 4] ^ D0, 27);
    const uint64_t B16 = rotate_left(A[ 5] ^ D1, 36);
    const uint64_t B01 = rotate_left(A[ 6] ^ D2, 44);
    const uint64_t B11 = rotate_left(A[ 7] ^ D3, 6);
    const uint64_t B21 = rotate_left(A[ 8] ^ D4, 55);
    const uint64_t B06 = rotate_left(A[ 9] ^ D0, 20);
    const uint64_t B07 = rotate_left(A[10] ^ D1, 3);
    const uint64_t B17 = rotate_left(A[11] ^ D2, 10);
    const uint64_t B02 = rotate_left(A[12] ^ D3, 43);
    const uint64_t B12 = rotate_left(A[13] ^ D4, 25);
    const uint64_t B22 = rotate_left(A[14] ^ D0, 39);
    const uint64_t B23 = rotate_left(A[15] ^ D1, 41);
    const uint64_t B08 = rotate_left(A[16] ^ D2, 45);
    const uint64_t B18 = rotate_left(A[17] ^ D3, 15);
    const uint64_t B03 = rotate_left(A[18] ^ D4, 21);
    const uint64_t B13 = rotate_left(A[19] ^ D0, 8);
    const uint64_t B14 = rotate_left(A[20] ^ D1, 18);
    const uint64_t B24 = rotate_left(A[21] ^ D2, 2);
    const uint64_t B09 = rotate_left(A[22] ^ D3, 61);
    const uint64_t B19 = rotate_left(A[23] ^ D4, 56);
    const uint64_t B04 = rotate_left(A[24] ^ D0, 14);

    A[ 0] = B00 ^ (~B01 & B02);
    A[ 1] = B01 ^ (~B02 & B03);
    A[ 2] = B02 ^ (~B03 & B04);
    A[ 3] = B03 ^ (~B04 & B00);
    A[ 4] = B04 ^ (~B00 & B01);
    A[ 5] = B05 ^ (~B06 & B07);
    A[ 6] = B06 ^ (~B07 & B08);
    A[ 7] = B07 ^ (~B08 & B09);
    A[ 8] = B08 ^ (~B09 & B05);
    A[ 9] = B09 ^ (~B05 & B06);
    A[10] = B10 ^ (~B11 & B12);
    A[11] = B11 ^ (~B12 & B13);
    A[12] = B12 ^ (~B13 & B14);
    A[13] = B13 ^ (~B14 & B10);
    A[14] = B14 ^ (~B10 & B11);
    A[15] = B15 ^ (~B16 & B17);
    A[16] = B16 ^ (~B17 & B18);
    A[17] = B17 ^ (~B18 & B19);
    A[18] = B18 ^ (~B19 & B15);
    A[19] = B19 ^ (~B15 & B16);
    A[20] = B20 ^ (~B21 & B22);
    A[21] = B21 ^ (~B22 & B23);
    A[22] = B22 ^ (~B23 & B24);
    A[23] = B23 ^ (~B24 & B20);
    A[24] = B24 ^ (~B20 & B21);
    A[0] ^= RC[i];
  }
}

static inline void xor_bytes(const uint8_t* in, uint8_t* out, size_t bytes)
   {
   for(size_t i = 0; i != bytes; ++i)
      out[i] ^= in[i];
   }

void rng_absorb(const void* input, size_t bytes)
  {
  size_t absorbed = 0;

  while(absorbed < bytes)
     {
     size_t absorbing = std::min<size_t>(bytes - absorbed, SHAKE_128_RATE);

     xor_bytes(static_cast<const uint8_t*>(input) + absorbed,
               reinterpret_cast<uint8_t*>(PER_CPU(rng).state),
               absorbing);

      keccak_1600_p(PER_CPU(rng).state);
      absorbed += absorbing;
     }
  PER_CPU(rng).reseed_counter += RESEED_RATE * bytes;
  }

static void reseed_now()
{
  uint64_t value;
  for (int i = 0; i < PER_CPU(rng).reseed_rounds; i++) {
      PER_CPU(rng).reseed_callback(&value);
      rng_absorb(&value, sizeof(value));
  }
}

void rng_extract(void* output, size_t bytes)
   {
   size_t copied = 0;

   while(copied < bytes)
      {
      size_t copying = std::min<size_t>(bytes - copied, SHAKE_128_RATE);
      memcpy(static_cast<uint8_t*>(output) + copied, PER_CPU(rng).state, copying);
      keccak_1600_p(PER_CPU(rng).state);
      copied += copying;
      }
    // don't reseed if no callback to do so
    if (PER_CPU(rng).reseed_callback == nullptr) return;
    PER_CPU(rng).reseed_counter -= bytes;
    // reseed when below certain entropy
    if (PER_CPU(rng).reseed_counter < 0) {
      PER_CPU(rng).reseed_counter = 0;
      reseed_now();
    }
   }

void rng_reseed_init(delegate<void(uint64_t*)> func, int rounds)
{
  PER_CPU(rng).reseed_callback = func;
  PER_CPU(rng).reseed_rounds = rounds;
  reseed_now();
}
