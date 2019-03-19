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
#include <arch.hpp>

namespace hw {

  template<class PCIInterface>
  class PCI_Handler
  {
  public:
    static inline uint8_t rdb(uint16_t port) {
      return PCIInterface::read_byte(port);
    }
    static inline uint16_t rdw(uint16_t port) {
      return PCIInterface::read_word(port);
    }
    static inline uint32_t rdl(uint16_t port) {
      return PCIInterface::read_long(port);
    }
    static inline void outb(uint16_t port,uint8_t data) {
      PCIInterface::write_byte(port,data);
    }
    static inline void outw(uint16_t port,uint16_t data) {
      PCIInterface::write_word(port,data);
    }
    static inline void outl(uint16_t port,uint32_t data) {
      PCIInterface::write_long(port,data);
    }
  };

  #if defined(ARCH_x86) || defined(ARCH_x86_64)
    class PCI_Impl
    {
    public:
      static inline uint8_t read_byte(uint16_t port)
      {
        uint8_t ret;
        asm volatile("inb %1,%0" : "=a"(ret) : "Nd"(port));
        return ret;
      }

      static inline uint16_t read_word(uint16_t port)
      {
        uint16_t ret;
        asm volatile("inw %1,%0" : "=a"(ret) : "Nd"(port));
        return ret;
      }

      static inline uint32_t read_long(uint16_t port)
      {
        uint32_t ret;
        asm volatile("inl %1,%0" : "=a"(ret) : "Nd"(port));
        return ret;
      }

      static inline void write_byte(uint16_t port, uint8_t data)
      {
        asm volatile ("outb %0,%1" :: "a"(data), "Nd"(port));
      }
      static inline void write_word(uint16_t port, uint16_t data)
      {
        asm volatile ("outw %0,%1" :: "a" (data), "Nd"(port));
      }
      static inline void write_long(uint16_t port, uint32_t data)
      {
        asm volatile ("outl %0,%1" :: "a" (data), "Nd"(port));
      }
    };

  #elif defined(ARCH_aarch64)
    class PCI_Impl
    {
    public:
      const static inline uint8_t read_byte(uint16_t port)
      {
        #warning NOT_IMPLEMENTED
        return 0;
      }

      static const inline uint16_t read_word(uint16_t port)
      {
        #warning NOT_IMPLEMENTED
        return 0;
      }

      static const inline uint32_t read_long(uint16_t port)
      {
        #warning NOT_IMPLEMENTED
        return 0;
      }

      static const inline void write_byte(uint16_t port, uint8_t data)
      {
          #warning NOT_IMPLEMENTED
      }
      static const inline void write_word(uint16_t port, uint16_t data)
      {
          #warning NOT_IMPLEMENTED
      }
      static const inline void write_long(uint16_t port, uint32_t data)
      {
          #warning NOT_IMPLEMENTED
      }
    };
  #endif
  static inline uint8_t inp(uint16_t port)
  {
    return PCI_Handler<PCI_Impl>::rdb(port);
  }

  static inline uint16_t inpw(uint16_t port)
  {
    return PCI_Handler<PCI_Impl>::rdw(port);
  }

  static inline uint32_t inpd(uint16_t port)
  {
    return PCI_Handler<PCI_Impl>::rdl(port);
  }

  static inline void outp(uint16_t port, uint8_t data)
  {
    PCI_Handler<PCI_Impl>::outb(port,data);
  }

  static inline void outpw(uint16_t port, uint16_t data)
  {
    PCI_Handler<PCI_Impl>::outw(port,data);
  }

  static inline void outpd(uint16_t port, uint32_t data)
  {
    PCI_Handler<PCI_Impl>::outl(port,data);
  }

} //< namespace hw

#endif
