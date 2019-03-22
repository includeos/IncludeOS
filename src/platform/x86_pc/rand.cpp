#include <kernel/rng.hpp>
#include <kernel/cpuid.hpp>
#include <kernel/os.hpp>
#include <arch.hpp>
#include <kprint>
extern "C" void intel_rdrand(void*);
extern "C" void intel_rdseed(void*);

// used to generate initial
// entropy for a cryptographic randomness generator
void RNG::init()
{
  if (CPUID::has_feature(CPUID::Feature::RDSEED)) {
    for (int i = 0; i < 64 * 2; i++)
    {
      uintptr_t rdseed;
      intel_rdseed(&rdseed);
      rng_absorb(&rdseed, sizeof(rdseed));
    }
  }
  else if (CPUID::has_feature(CPUID::Feature::RDRAND)) {
    for (int i = 0; i < 64 * 5; i++)
    {
      uintptr_t rdrand;
      intel_rdrand(&rdrand);
      rng_absorb(&rdrand, sizeof(rdrand));
    }
  }
  else {
    for (int i = 0; i < 64 * 16; i++)
    {
      uint64_t clock = 0;
      // this is horrible, better solution needed here
      for (int i = 0; i < 64; ++i) {
        clock += OS::cycles_since_boot();
        asm volatile("cpuid" ::: "memory", "eax", "ebx", "ecx", "edx");
      }
      rng_absorb(&clock, sizeof(clock));
    }
    return;
  }
  assert(0 && "No randomness fallback");
}
