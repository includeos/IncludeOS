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

#ifndef X86_64_ARCH_HPP
#define X86_64_ARCH_HPP

#define ARCH_x86

inline void __arch_read_memory_barrier() noexcept {
  __asm volatile("lfence" ::: "memory");
}
inline void __arch_write_memory_barrier() noexcept {
  __asm volatile("mfence" ::: "memory");
}

inline uint64_t __arch_cpu_cycles() noexcept {
  uint32_t hi, lo;
  asm("rdtsc" : "=a"(lo), "=d"(hi));
  return ((uint64_t) lo) | ((uint64_t) hi) << 32;
}

constexpr uintptr_t __arch_max_canonical_addr = 0xffffffffffff;

#endif
