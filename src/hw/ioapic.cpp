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

#include <hw/ioapic.hpp>
#include <cstdio>
#include <debug>
#include <info>

// I/O APIC registers
#define REG_IOAPICID    0
#define REG_IOAPICVER   1
#define REG_IOAPICARB   2
#define REG_IOREDTBL    3

namespace hw
{
  struct ioapic
  {
    uint32_t read(uint8_t reg) const noexcept {
      auto volatile* addr = (uint32_t volatile*) this->base;
      addr[0] = reg;
      return addr[4];
    }
    void write(uint8_t reg, uint32_t value) {
      auto volatile* addr = (uint32_t volatile*) this->base;
      addr[0] = reg;
      addr[4] = value;
    }
    void set_entry(uint8_t index, uint64_t data)
    {
      write(REG_IOREDTBL + index * 2,     (uint32_t) data);
      write(REG_IOREDTBL + index * 2 + 1, (uint32_t) (data >> 32));
    }
    
    void init(uintptr_t base)
    {
      this->base = base;
      // number of redirection entries supported
      entries = read(REG_IOAPICVER) >> 16;
      entries = 1 + (entries & 0xff);
      
      INFO("IOAPIC", "Initializing");
      INFO2("Base addr: 0x%x  Entries: %u", base, entries);
      
      // default: all entries disabled
      for (uint32_t i = 0; i < entries; i++)
        set_entry(i, IOAPIC_IRQ_DISABLE);
      
      INFO("IOAPIC", "Done");
    }
    
    uintptr_t  base;
    uint32_t   entries;
  };
  // there can be more than one IOAPIC
  static ioapic numbawan;
  
  void IOAPIC::init(const ACPI::ioapic_list& vec)
  {
    numbawan.init(vec[0].addr_base);
  }
  
}
