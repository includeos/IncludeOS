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
#ifndef HW_ACPI_HPP
#define HW_ACPI_HPP

#include <cstdint>
#include <vector>

namespace hw {
  
  class ACPI {
  public:
    struct LAPIC {
      uint8_t  type;
      uint8_t  length;
      uint8_t  cpu;
      uint8_t  id;
      uint32_t flags;
    };
    struct IOAPIC {
      uint8_t   type;
      uint8_t   length;
      uint8_t   id;
      uint8_t   reserved;
      uintptr_t addr_base;
      uintptr_t intr_base;
    };
    struct override_t {
      uint8_t   type;
      uint8_t   length;
      uint8_t   bus_source;
      uint8_t   irq_source;
      uint32_t  global_intr;
      uint16_t  flags;
    } __attribute__((packed));
    
    typedef std::vector<LAPIC> lapic_list;
    typedef std::vector<IOAPIC> ioapic_list;
    typedef std::vector<override_t> override_list;
    
    static void init() {
      get().discover();
    }
    
    static uint64_t time();
    
    static ACPI& get() {
      static ACPI acpi;
      return acpi;
    }
    
    static const lapic_list& get_cpus() {
      return get().lapics;
    }
    static const ioapic_list& get_ioapics() {
      return get().ioapics;
    }
    static const override_list& get_overrides() {
      return get().overrides;
    }
    
  private:
    void discover();
    bool checksum(const char*, size_t) const;
    void begin(const void* addr);
    
    void walk_sdts(const char* addr);
    void walk_madt(const char* addr);
    
    uintptr_t hpet_base;
    uintptr_t apic_base;
    ioapic_list   ioapics;
    lapic_list    lapics;
    override_list overrides;
  };
  
}

#endif
