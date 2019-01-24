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

#if !defined(AARCH64_ARCH_HPP)
#define AARCH64_ARCH_HPP

//TODO VERIFY
//2^47

inline uint64_t __arch_cpu_cycles() noexcept {
  uint64_t ret;
  //isb then read
  asm volatile("isb;mrs %0, pmccntr_el0" : "=r"(ret));
  return ret;
}

constexpr uintptr_t __arch_max_canonical_addr = 0x7ffffffffff;

#endif //AARCH64_ARCH_HPP
