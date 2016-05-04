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
    uint32_t u32;
  };
  
  struct apic_registers
  {
    apic_reg reserv1[2]; // 0000h - 0010h
    
    apic_reg lapic_id;   // 0020h ID
    apic_reg lapic_ver;  // 0030h VER
    apic_reg reserv2[4]; // 0040h - 0070h
    
    apic_reg task_prio;  // 0080h TPR
    apic_reg arbi_prio;  // 0090h APR
    apic_reg proc_prio;  // 00A0h PPR
    apic_reg eoi;        // 00B0h EOI
    apic_reg remt_read;  // 00C0h RRD
    apic_reg logic_dst;  // 00D0h
    apic_reg dest_fmt;   // 00E0h
    
    apic_reg spur_intv;  // 00F0h
    
    // In-Service Registers (ISRs)
    apic_reg isr[8];     // 0100h - 0170h ISR
    
    // Trigger Mode Registers (TMRs)
    apic_reg trig[8];    // 0180h - 01F0h TMR
    
    // Interrupt Request Registers (IRRs)
    apic_reg irr[8];     // 0200h - 0270h TMR
    
    apic_reg error_stat; // 0280h
    apic_reg reserv3[6]; // 0290h - 02F0h
    
    apic_reg lvt_cmci;   // 02F0h
    
    apic_reg intr_cmd0;  // 0300h ICR
    apic_reg intr_cmd1;  // 0310h ICR
    
    apic_reg lvt_timer;  // 0320h
    apic_reg lvt_therm;  // 0330h
    apic_reg lvt_perf;   // 0340h
    apic_reg lvt_lint0;  // 0350h
    apic_reg lvt_lint1;  // 0360h
    apic_reg lvt_error;  // 0370h
    // timer initial and current counters
    apic_reg initial_cnt; // 0380h  RW
    apic_reg current_cnt; // 0390h  RO
    apic_reg reserv4[4]; // 03A0h - 03E0h
    
    apic_reg div_config; // 03E0h   RW
    apic_reg reserv5;    // 03F0h
    
    bool x2apic() const noexcept {
      return false;
    }
    
    uint32_t get_id() const noexcept {
      return (lapic_id.u32 >> 24) & 0xFF;
    }
    
  };
  
  void APIC::init() {
    
    const uint64_t APIC_BASE_MSR = RDMSR(IA32_APIC_BASE);
    
    // find the LAPICs base address
    const uintptr_t APIC_BASE_ADDR = APIC_BASE_MSR & 0xFFFFF000;
    printf("APIC base addr: 0x%x\n", APIC_BASE_ADDR);
    // acquire infos
    auto* regs = (apic_registers*) APIC_BASE_ADDR;
    printf("LAPIC id: 0x%x\n", regs->get_id());
    
  }
  
}
