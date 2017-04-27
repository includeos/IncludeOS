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
#ifndef X86_CPU_HPP
#define X86_CPU_HPP

#include <cstdint>
#include <arch.hpp>

namespace x86
{
  class CPU
  {
  public:
    // read the intel manual
    static uint64_t
    read_msr(uint32_t addr)
    {

#if defined(ARCH_x86)
      uint64_t v;
      asm volatile("rdmsr": "=A" (v) : "c" (addr));
      return v;
#else
#error "read_msr() not implemented for selected arch"
#endif
    }

    static void
    write_msr(uint32_t addr, uint32_t eax, uint32_t edx)
    {
#if defined(ARCH_x86)
      asm volatile("wrmsr" : : "a" (eax), "d"(edx), "c" (addr));
#else
#error "write_msr() not implemented for selected arch"
#endif
    }

    static void
    write_msr(uint32_t addr, uint64_t value)
    {
#if defined(ARCH_x86_64)
      const uint32_t eax = value & 0xffffffff;
      const uint32_t edx = value >> 32;
      asm volatile("wrmsr" : : "a" (eax), "d"(edx), "c" (addr));
#elif defined(ARCH_x86)
      asm volatile("wrmsr" : : "A" (value), "c" (addr));
#else
#error "write_msr() not implemented for selected arch"
#endif
    }
  };
}

#endif
