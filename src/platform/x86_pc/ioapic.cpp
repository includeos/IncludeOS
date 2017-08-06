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

#include "ioapic.hpp"
#include <hw/ioport.hpp>
#include <cstdio>
#include <cassert>
#include <debug>
#include <info>

// I/O APIC registers
#define IOAPIC_ID    0
#define IOAPIC_VER   1
#define IOAPIC_ARB   2
#define IOAPIC_RDT   3

#define IOAPIC_INTR  0x10

namespace x86
{
  struct ioapic
  {
    __attribute__((noinline))
    uint32_t read(uint32_t reg) noexcept {
      *(volatile uint32_t*) (base + 0x0)  = reg;
      return *(volatile uint32_t*) (base + 0x10);
    }
    __attribute__((noinline))
    void write(uint32_t reg, uint32_t value) noexcept {
      *(volatile uint32_t*) (base + 0x0)  = reg;
      *(volatile uint32_t*) (base + 0x10) = value;
    }
    void set_entry(uint8_t index, uint32_t lo, uint32_t hi)
    {
      assert(index < entries());
      write(IOAPIC_INTR + index * 2 + 1, hi);
      write(IOAPIC_INTR + index * 2,     lo);
    }
    void set_id(uint32_t id) noexcept
    {
      write(IOAPIC_ID, id << 24);
    }
    uint32_t get_id() noexcept
    {
      return read(IOAPIC_ID) >> 24;
    }

    uint8_t entries() const noexcept
    {
      return entries_;
    }

    void init(uintptr_t base_addr)
    {
      this->base = base_addr;
      // required: set IOAPIC ID
      set_id(0);
      // print base addr and version
      uint8_t version = read(IOAPIC_VER) & 0xff;
      INFO2("Base addr: %p  Version: %#x", (void*) base, version);

      // number of redirection entries supported
      uint32_t reg = read(IOAPIC_VER) >> 16;
      this->entries_ = 1 + (reg & 0xff);

      INFO2("ID: %u  Redirection entries: %u", get_id(), entries());

      // default: all entries disabled
      for (unsigned i = 0; i < entries(); i++)
        set_entry(i, IOAPIC_IRQ_DISABLE, 0);
    }

  private:
    uintptr_t base;
    uint32_t  entries_;
  };
  // there can be more than one IOAPIC
  static ioapic numbawan;

  void IOAPIC::init(const ACPI::ioapic_list& vec)
  {
    INFO("I/O APIC", "Initializing");
    // enable IO APIC (ICMR wiring mode?)
    hw::outb(0x22, 0x70);
    hw::outb(0x23, 0x1);

    numbawan.init(vec.at(0).addr_base);
  }

  unsigned IOAPIC::entries()
  {
    return numbawan.entries();
  }

  void IOAPIC::set(uint8_t idx, uint32_t src, uint32_t dst)
  {
    numbawan.set_entry(idx, src, dst);
  }

  void IOAPIC::enable(uint8_t idx, uint8_t irq_dst, uint8_t dst)
  {
    uint32_t type = 0;

    if (idx > 15)
        type = (1<<15) | (1<<13);

    numbawan.set_entry(idx, type | (0x20 + irq_dst), dst);
  }
  void IOAPIC::disable(uint8_t idx)
  {
    uint32_t type = 1 << 16;

    if (idx > 15)
      type |= (1<<15) | (1<<13);

    numbawan.set_entry(idx, type | (0x20 + idx), 0x0);
  }
}
