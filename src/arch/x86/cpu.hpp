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
#ifndef x86_CPU_HPP
#define x86_CPU_HPP

#include <cstdint>

namespace x86
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
    static void
    write_msr(uint32_t addr, uint64_t value)
    {
      union {
        uint64_t value;
        uint32_t reg[2];
      } msr {value};
      asm volatile("wrmsr" : : "a" (msr.reg[0]),"d"(msr.reg[1]), "c" (addr));
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
