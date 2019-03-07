#include <kernel/cpuid.hpp>
#include <os.hpp>
#include <arch.hpp>
#include <kprint>
extern "C" void intel_rdrand(void*);

// this function uses a combination of the best randomness
// sources the platform has to offer, to generate initial
// entropy for a cryptographic randomness generator
uint32_t __arch_rand32()
{
  if (CPUID::has_feature(CPUID::Feature::RDRAND)) {
    uintptr_t rdrand;
    intel_rdrand(&rdrand);
    return rdrand;
  }
  else {
    uint64_t clock = 0;
    // this is horrible, better solution needed here
    for (int i = 0; i < 64; ++i) {
      clock += os::cycles_since_boot();
      asm volatile("cpuid" ::: "memory", "eax", "ebx", "ecx", "edx");
    }
    return clock;
  }
  assert(0 && "No randomness fallback");
}
