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
#ifndef HW_PCI_HPP
#define HW_PCI_HPP

#include <cstdint>

namespace hw {

  typedef uint16_t port_t;

  static inline uint8_t inp(port_t port)
  {
    uint8_t ret;
  
    __asm__ volatile("xorl %eax,%eax");
    __asm__ volatile("inb %%dx,%%al"
                     :"=a"(ret)
                     :"d"(port));
    return ret;  
  }

  static inline uint16_t inpw(port_t port)
  {
    uint16_t ret;
    __asm__ volatile("xorl %eax,%eax");
    __asm__ volatile("inw %%dx,%%ax"
                     :"=a"(ret)
                     :"d"(port));
    return ret;    
  }

  static inline uint32_t inpd(port_t port)
  {
    uint32_t ret;
    __asm__ volatile("xorl %eax,%eax");
    __asm__ volatile("inl %%dx,%%eax"
                     :"=a"(ret)
                     :"d"(port));
  
    return ret;
  }


  static inline void outp(port_t port, uint8_t data)
  {
    __asm__ volatile ("outb %%al,%%dx"::"a" (data), "d"(port));
  }
  static inline void outpw(port_t port, uint16_t data)
  {
    __asm__ volatile ("outw %%ax,%%dx"::"a" (data), "d"(port));
  }
  static inline void outpd(port_t port, uint32_t data)
  {
    __asm__ volatile ("outl %%eax,%%dx"::"a" (data), "d"(port));
  }

} //< namespace hw

#endif
