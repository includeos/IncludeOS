// -*-C++-*-

#ifndef AARCH64_ARCH_HPP
#define AARCH64_ARCH_HPP

#ifndef ARCH_aarch64
  #define ARCH_aarch64
#endif

//TODO VERIFY
//2^47
namespace os {

  // Concept / base class Arch
  struct Arch {
    static constexpr uintptr_t max_canonical_addr = 0x7ffffffffff;
    static constexpr uint8_t     word_size          = sizeof(uintptr_t) * 8;
    static constexpr uintptr_t   min_page_size      = 4096;
    static constexpr const char* name               = "aarch64";
    static inline uint64_t cpu_cycles() noexcept;
  };
}
//IMPL
constexpr uintptr_t __arch_max_canonical_addr = 0x7ffffffffff;
uint64_t os::Arch::cpu_cycles() noexcept {
  uint64_t ret;
  //isb then read
  asm volatile("isb;mrs %0, pmccntr_el0" : "=r"(ret));
  return ret;
}
#endif //AARCH64_ARCH_HPP
