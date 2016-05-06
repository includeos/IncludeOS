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

#include <hw/apic.hpp>
#include <hw/ioport.hpp>
#include <utility/membitmap.hpp>

#define LAPIC_ID        0x20
#define LAPIC_VER       0x30
#define LAPIC_TPR       0x80
#define LAPIC_EOI       0x0B0
#define LAPIC_LDR       0x0D0
#define LAPIC_DFR       0x0E0
#define LAPIC_SPURIOUS  0x0F0
#define LAPIC_ESR	      0x280
#define LAPIC_ICRL      0x300
#define LAPIC_ICRH      0x310
#define LAPIC_LVT_TMR	  0x320
#define LAPIC_LVT_PERF  0x340
#define LAPIC_LVT_LINT0 0x350
#define LAPIC_LVT_LINT1 0x360
#define LAPIC_LVT_ERR   0x370
#define LAPIC_TMRINITCNT 0x380
#define LAPIC_TMRCURRCNT 0x390
#define LAPIC_TMRDIV    0x3E0
#define LAPIC_LAST      0x38F
#define LAPIC_DISABLE   0x10000
#define LAPIC_SW_ENABLE 0x100
#define LAPIC_CPUFOCUS  0x200
#define LAPIC_NMI       (4<<8)
#define TMR_PERIODIC    0x20000
#define TMR_BASEDIV     (1<<20)

extern "C" {
  void apic_enable();
}

namespace hw {
  
  static const uintptr_t IA32_APIC_BASE = 0x1B;
  
  uint64_t RDMSR(uint32_t addr)
  {
    uint32_t EAX = 0, EDX = 0;
    asm volatile("rdmsr": "=a" (EAX),"=d"(EDX) : "c" (addr));
    return ((uint64_t)EDX << 32) | EAX;
  }
  
  // a single 16-byte aligned APIC register
  typedef uint32_t apic_reg;
  
  struct apic
  {
    apic() {}
    apic(uintptr_t addr)
      : base_addr(addr) {}
    
    bool x2apic() const noexcept {
      return false;
    }
    
    apic_reg get_id() const noexcept {
      return (read(LAPIC_ID) >> 24) & 0xFF;
    }
    
    uint32_t read(apic_reg reg) const noexcept {
      auto volatile* addr = (uint32_t volatile*) base_addr;
      addr[0] = reg & 0xff;
      return addr[4];
    }
    void write(apic_reg reg, uint32_t value) {
      auto volatile* addr = (uint32_t volatile*) base_addr;
      addr[0] = reg & 0xff;
      addr[4] = value;
    }
    
    uintptr_t base_addr;
  };
  
  static apic lapic;
  
  void APIC::init() {
    
    const uint64_t APIC_BASE_MSR = RDMSR(IA32_APIC_BASE);
    
    // find the LAPICs base address
    const uintptr_t APIC_BASE_ADDR = APIC_BASE_MSR & 0xFFFFF000;
    printf("APIC base addr: 0x%x\n", APIC_BASE_ADDR);
    // acquire infos
    lapic = apic(APIC_BASE_ADDR);
    printf("LAPIC id: 0x%x\n", lapic.get_id());
    
    
    apic_enable();
    
  }
  
}
