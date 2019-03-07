// -*-C++-*-
// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017 Oslo and Akershus University College of Applied Sciences
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
