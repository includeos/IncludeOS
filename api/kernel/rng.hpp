// -*-C++-*-
// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2016-2018 Oslo and Akershus University College of Applied Sciences
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
#ifndef KERNEL_RNG_HPP
#define KERNEL_RNG_HPP

#include <cstdlib>
#include <cstdint>
#include <delegate>

// Incorporate seed data into the system RNG state
extern void rng_absorb(const void* input, size_t bytes);

// Extract output from the system RNG
extern void rng_extract(void* output, size_t bytes);

// Try to reseed the RNG state
extern void rng_reseed_init(delegate<void(uint64_t*)>, int rounds);

// Extract 32 bit integer from system RNG
inline uint32_t rng_extract_uint32()
  {
  uint32_t x;
  rng_extract(&x, sizeof(x));
  return x;
  }

// Extract 64 bit integer from system RNG
inline uint64_t rng_extract_uint64()
  {
  uint64_t x;
  rng_extract(&x, sizeof(x));
  return x;
  }

#include <fs/fd_compatible.hpp>
class RNG : public FD_compatible {
public:
  static RNG& get()
  {
    static RNG rng;
    return rng;
  }

  static void init();

private:
  RNG() {}

};


#endif
