// -*-C++-*-

#pragma once
#ifndef KERNEL_RNG_HPP
#define KERNEL_RNG_HPP

#define INCLUDEOS_RNG_IS_SHARED

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
