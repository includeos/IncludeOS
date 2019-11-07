// -*-C++-*-

#ifndef i686_ARCH_HPP
#define i686_ARCH_HPP

#define ARCH_x86


namespace os {

  // Concept / base class Arch
  struct Arch {
    static constexpr uintptr_t   max_canonical_addr = 0xffffffff;
    static constexpr uint8_t     word_size          = sizeof(uintptr_t) * 8;
    static constexpr uintptr_t   min_page_size      = 4096;
    static constexpr const char* name               = "i686";

    static inline uint64_t cpu_cycles() noexcept;
    static inline void read_memory_barrier() noexcept;
    static inline void write_memory_barrier() noexcept;
  };

}

// Implementation
inline uint64_t os::Arch::cpu_cycles() noexcept {
  uint64_t ret;
  asm("rdtsc" : "=A" (ret));
  return ret;
}

inline void os::Arch::read_memory_barrier() noexcept {
  __asm volatile("lfence" ::: "memory");
}

inline void os::Arch::write_memory_barrier() noexcept {
  __asm volatile("mfence" ::: "memory");
}

#endif
