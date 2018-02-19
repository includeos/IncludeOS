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
#define IRQ_BASE     32

namespace x86
{
  struct IOapic
  {
    inline uint32_t read(uint32_t reg) noexcept {
      *(volatile uint32_t*) (m_base + 0x0)  = reg;
      return *(volatile uint32_t*) (m_base + 0x10);
    }
    inline void write(uint32_t reg, uint32_t value) noexcept {
      *(volatile uint32_t*) (m_base + 0x0)  = reg;
      *(volatile uint32_t*) (m_base + 0x10) = value;
    }
    void set_entry(uint8_t entry, uint32_t lo, uint32_t hi)
    {
      assert(entry < entries());
      write(IOAPIC_INTR + entry * 2 + 1, hi);
      write(IOAPIC_INTR + entry * 2,     lo);
    }
    void set_id(uint32_t id) noexcept
    {
      write(IOAPIC_ID, id << 24);
    }
    uint8_t get_id() noexcept
    {
      return read(IOAPIC_ID) >> 24;
    }

    uint32_t intr_base() const noexcept
    {
      return m_intr_base;
    }
    uint32_t entries() const noexcept
    {
      return m_entries;
    }

    IOapic(uint8_t idx, uintptr_t base_addr, uint32_t intr_base)
      : m_base(base_addr), m_intr_base(intr_base)
    {
      INFO("I/O APIC", "Initializing ID=%u", idx);
      // required: set IOAPIC ID
      set_id(0);
      // print base addr and version
      uint8_t version = read(IOAPIC_VER) & 0xff;
      // number of redirection entries supported
      uint32_t reg = read(IOAPIC_VER) >> 16;
      this->m_entries = 1 + (reg & 0xff);

      INFO2("Addr: %p  Version: %#x  Intr: %u  Entries: %u",
            (void*) base_addr, version, intr_base, m_entries);

      // default: all entries disabled
      for (unsigned i = 0; i < entries(); i++)
        set_entry(i, IOAPIC_IRQ_DISABLE, 0);
    }

  private:
    uintptr_t m_base;
    uint32_t  m_intr_base;
    uint32_t  m_entries;
  };
  static std::vector<IOapic> ioapics;

  void IOAPIC::init(const ACPI::ioapic_list& vec)
  {
    // enable IO APIC (ICMR wiring mode?)
    hw::outb(0x22, 0x70);
    hw::outb(0x23, 0x1);

    ioapics.reserve(vec.size());
    for (auto& ioapic : vec) {
      ioapics.emplace_back(ioapic.id, ioapic.addr_base, ioapic.intr_base);
    }
  }

  inline IOapic& get_ioapic_for(uint32_t entry)
  {
    uint32_t current = 0;
    for (auto& a : ioapics) {
      if (entry >= current && entry < current + a.entries()) {
        return a;
      }
      current += a.entries();
    }
    printf("Entry: %u\n", entry);
    assert(0 && "Could not match I/O APIC to entry");
  }

  void IOAPIC::enable(uint8_t cpu, const ACPI::override_t& redir)
  {
    int idx = redir.global_intr;
    auto& ioa = get_ioapic_for(idx);
    idx -= ioa.intr_base();

    uint32_t field = IRQ_BASE + redir.irq_source;
    if (idx > 15)
        field |= (1<<15) | (1<<13);

    ioa.set_entry(idx, field, cpu);
  }
  void IOAPIC::disable(uint8_t idx)
  {
    auto& ioa = get_ioapic_for(idx);
    ioa.set_entry(idx, IOAPIC_IRQ_DISABLE, 0x0);
  }
}
