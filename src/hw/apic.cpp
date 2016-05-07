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
  struct apic_reg
  {
    uint32_t reg;
    uint32_t pad[3];
  };
  
  struct apic_regs
  {
    apic_reg reserved0;
    apic_reg reserved1;
    apic_reg 		lapic_id;
    apic_reg 		lapic_ver;
    apic_reg reserved4;
    apic_reg reserved5;
    apic_reg reserved6;
    apic_reg reserved7;
    apic_reg 		task_pri;               // TPRI
    apic_reg reservedb; // arb pri
    apic_reg reservedc; // cpu pri
    apic_reg 		eoi;                    // EOI
    apic_reg 		remote;
    apic_reg 		logical_dest;
    apic_reg 		dest_format;
    apic_reg 		spurious_vector;        // SIVR
    apic_reg 		isr[8];
    apic_reg 		tmr[8];
    apic_reg 		irr[8];
    apic_reg    error_status;
    apic_reg reserved28[7];
    apic_reg			int_command[2]; 	// ICR1. ICR2
    apic_reg 		timer_vector; 		// LVTT
    apic_reg reserved33;
    apic_reg reserved34; // perf count lvt
    apic_reg 		lint0_vector;
    apic_reg 		lint1_vector;
    apic_reg 		error_vector; // err vector
    apic_reg 		init_count; // timer
    apic_reg 		cur_count;  // timer
    apic_reg reserved3a;
    apic_reg reserved3b;
    apic_reg reserved3c;
    apic_reg reserved3d;
    apic_reg 		divider_config; // 3e, timer divider
    apic_reg reserved3f;
  };
  
  struct apic
  {
    apic() {}
    apic(uintptr_t addr)
    {
      this->regs = (apic_regs*) addr;
    }
    
    bool x2apic() const noexcept {
      return false;
    }
    
    uint32_t get_id() const noexcept {
      return (regs->lapic_id.reg >> 24) & 0xFF;
    }
    
    uint32_t io_read(uint32_t reg) const noexcept {
      auto volatile* addr = (uint32_t volatile*) ioapic_base;
      addr[0] = reg & 0xff;
      return addr[4];
    }
    void io_write(uint32_t reg, uint32_t value) {
      auto volatile* addr = (uint32_t volatile*) ioapic_base;
      addr[0] = reg & 0xff;
      addr[4] = value;
    }
    
    // set and clear one of the 255-bit registers
    void set(apic_reg* reg, uint8_t bit)
    {
      reg[bit >> 5].reg |= 1 << (bit & 0x1f);
    }
    void clr(apic_reg* reg, uint8_t bit)
    {
      reg[bit >> 5].reg &= ~(1 << (bit & 0x1f));
    }
    
    apic_regs* regs;
    uintptr_t  ioapic_base;
  };
  
  static apic lapic;
  
  void APIC::init() {
    
    const uint64_t APIC_BASE_MSR = RDMSR(IA32_APIC_BASE);
    
    // find the LAPICs base address
    const uintptr_t APIC_BASE_ADDR = APIC_BASE_MSR & 0xFFFFF000;
    printf("APIC base addr: 0x%x\n", APIC_BASE_ADDR);
    // acquire infos
    lapic = apic(APIC_BASE_ADDR);
    printf("LAPIC id: %x  ver: %x\n", lapic.get_id(), lapic.regs->lapic_ver.reg);
    
    
    apic_enable();
    
  }
  
}
