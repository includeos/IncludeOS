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

#ifndef HW_IOPORT_HPP
#define HW_IOPORT_HPP

#include <common>
#include <arch.hpp>

namespace hw {

  static inline uint8_t inb(uint16_t port)
  {
    uint8_t ret;
#if defined(ARCH_x86)
    asm volatile("inb %1,%0" : "=a"(ret) : "Nd"(port));
#elif defined(ARCH_aarch64)
#else
#error "inp() not implemented for selected arch"
#endif
    return ret;
  }

  static inline uint16_t inw(uint16_t port)
  {
    uint16_t ret;
#if defined(ARCH_x86)
    asm volatile("inw %1,%0" : "=a"(ret) : "Nd"(port));
#elif defined(ARCH_aarch64)
#else
#error "inpw() not implemented for selected arch"
#endif
    return ret;
  }

  static inline uint32_t inl(uint16_t port)
  {
    uint32_t ret;
#if defined(ARCH_x86)
    asm volatile("inl %1,%0" : "=a"(ret) : "Nd"(port));
#elif defined(ARCH_aarch64)
#else
#error "inpd() not implemented for selected arch"
#endif
    return ret;
  }

  static inline void outb(uint16_t port, uint8_t data)
  {
#if defined(ARCH_x86)
    asm volatile ("outb %0,%1" :: "a"(data), "Nd"(port));
#elif defined(ARCH_aarch64)
#else
#error "outp() not implemented for selected arch"
#endif
  }
  static inline void outw(uint16_t port, uint16_t data)
  {
#if defined(ARCH_x86)
    asm volatile ("outw %0,%1" :: "a" (data), "Nd"(port));
#elif defined(ARCH_aarch64)
#else
#error "outpw() not implemented for selected arch"
#endif
  }
  static inline void outl(uint16_t port, uint32_t data)
  {
#if defined(ARCH_x86)
    asm volatile ("outl %0,%1" :: "a" (data), "Nd"(port));
#elif defined(ARCH_aarch64)
#else
#error "outpd() not implemented for selected arch"
#endif
  }

} //< namespace hw

#endif // HW_IOPORT_HPP
