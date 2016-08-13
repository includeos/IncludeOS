// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
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

#pragma once
#ifndef HW_CPU_HPP
#define HW_CPU_HPP

#include <cstdint>

namespace hw
{
  class CPU
  {
  public:
    // read the intel manual
    static uint64_t
    read_msr(uint32_t addr)
    {
      uint32_t EAX = 0, EDX = 0;
      asm volatile("rdmsr": "=a" (EAX),"=d"(EDX) : "c" (addr));
      return ((uint64_t)EDX << 32) | EAX;
    }
    
    static void
    write_msr(uint32_t addr, uint32_t eax, uint32_t edx)
    {
      asm volatile("wrmsr" : : "a" (eax),"d"(edx), "c" (addr));
    }
    
    static uint64_t rdtsc()
    {
      uint64_t ret;
      __asm__ volatile ("rdtsc":"=A"(ret));
      return ret;
    }
  };
}

#endif
