#include <kernel/rng.hpp>
#include <kernel/cpuid.hpp>
#include <os.hpp>
#include <arch.hpp>
#include <kprint>
extern "C" void intel_rdrand(uint64_t*);
extern "C" void intel_rdseed(uint64_t*);

static void fallback_entropy(uint64_t* res)
{
  uint64_t clock = (uint64_t) res;
  // this is horrible, better solution needed here
  for (int i = 0; i < 64; ++i) {
    clock += os::cycles_since_boot();
    asm volatile("cpuid" ::: "memory", "eax", "ebx", "ecx", "edx");
  }
  // here we need to add our own bits
  *res = clock;
}

// used to generate initial
// entropy for a cryptographic randomness generator
void RNG::init()
{
  if (CPUID::has_feature(CPUID::Feature::RDSEED)) {
    rng_reseed_init(intel_rdseed, 2);
    return;
  }
  else if (CPUID::has_feature(CPUID::Feature::RDRAND)) {
    rng_reseed_init(intel_rdrand, 65);
    return;
  }
#ifndef PLATFORM_x86_solo5
  rng_reseed_init(fallback_entropy, 64*16);
  return;
#endif
  assert(0 && "No randomness fallback");
}
