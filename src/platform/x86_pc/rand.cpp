#include <kernel/rng.hpp>
#include <kernel/cpuid.hpp>
#include <kernel/os.hpp>
#include <arch.hpp>
#include <kprint>
extern "C" void intel_rdrand(void*);
extern "C" void intel_rdseed(void*);

// used to generate initial
// entropy for a cryptographic randomness generator
static uint64_t hwrand64()
{
  if (CPUID::has_feature(CPUID::Feature::RDSEED)) {
    uintptr_t rdseed;
    intel_rdrand(&rdseed);
    return rdseed;
  }
  else if (CPUID::has_feature(CPUID::Feature::RDRAND)) {
    uintptr_t rdrand;
    intel_rdrand(&rdrand);
    return rdrand;
  }
  else {
    uint64_t clock = 0;
    // this is horrible, better solution needed here
    for (int i = 0; i < 64; ++i) {
      clock += OS::cycles_since_boot();
      asm volatile("cpuid" ::: "memory", "eax", "ebx", "ecx", "edx");
    }
    return clock;
  }
  assert(0 && "No randomness fallback");
}

void RNG::init()
{
  for (int i = 0; i < 64; i++)
  {
    const uint64_t bits = hwrand64();
    rng_absorb(&bits, sizeof(bits));
  }
}
