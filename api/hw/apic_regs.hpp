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
#ifndef HW_APIC_REGS_HPP
#define HW_APIC_REGS_HPP

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

#define INTR_MASK    0x00010000

#define LAPIC_IRQ_BASE   120
#define LAPIC_IRQ_TIMER  (LAPIC_IRQ_BASE+0)
#define LAPIC_IRQ_LINT0  (LAPIC_IRQ_BASE+3)
#define LAPIC_IRQ_LINT1  (LAPIC_IRQ_BASE+4)
#define LAPIC_IRQ_ERROR  (LAPIC_IRQ_BASE+5)

// Interrupt Command Register
#define ICR_DEST_BITS   24

// Delivery Mode
#define ICR_FIXED               0x000000
#define ICR_LOWEST              0x000100
#define ICR_SMI                 0x000200
#define ICR_NMI                 0x000400
#define ICR_INIT                0x000500
#define ICR_STARTUP             0x000600

// Destination Mode
#define ICR_PHYSICAL            0x000000
#define ICR_LOGICAL             0x000800

// Delivery Status
#define ICR_IDLE                0x000000
#define ICR_SEND_PENDING        0x001000
#define ICR_DLV_STATUS          (1u <<12)

// Level
#define ICR_DEASSERT            0x000000
#define ICR_ASSERT              0x004000

// Trigger Mode
#define ICR_EDGE                0x000000
#define ICR_LEVEL               0x008000

// Destination Shorthand
#define ICR_NO_SHORTHAND        0x000000
#define ICR_SELF                0x040000
#define ICR_ALL_INCLUDING_SELF  0x080000
#define ICR_ALL_EXCLUDING_SELF  0x0c0000

namespace hw
{
  // a single 16-byte aligned APIC register
  struct apic_reg
  {
    uint32_t volatile reg;
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
    apic_reg		intr_lo; 	    // ICR0
    apic_reg		intr_hi;      // ICR1
    apic_reg 		timer;        // LVTT
    apic_reg reserved33;
    apic_reg reserved34;      // perf count lvt
    apic_reg 		lint0;        // local interrupts (64-bit)
    apic_reg 		lint1;
    apic_reg 		error;        // error vector
    apic_reg 		init_count;   // timer
    apic_reg 		cur_count;    // timer
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

    // set and clear one of the 255-bit registers
    void set(apic_reg* reg, uint8_t bit)
    {
      reg[bit >> 5].reg |= 1 << (bit & 0x1f);
    }
    void clr(apic_reg* reg, uint8_t bit)
    {
      reg[bit >> 5].reg &= ~(1 << (bit & 0x1f));
    }

    // initialize a given AP
    void ap_init(uint8_t id)
    {
      regs->intr_hi.reg = id << ICR_DEST_BITS;
      regs->intr_lo.reg = ICR_INIT | ICR_PHYSICAL
           | ICR_ASSERT | ICR_EDGE | ICR_NO_SHORTHAND;

      while (regs->intr_lo.reg & ICR_SEND_PENDING);
    }
    void ap_start(uint8_t id, uint32_t vector)
    {
      regs->intr_hi.reg = id << ICR_DEST_BITS;
      regs->intr_lo.reg = vector | ICR_STARTUP
        | ICR_PHYSICAL | ICR_ASSERT | ICR_EDGE | ICR_NO_SHORTHAND;

      while (regs->intr_lo.reg & ICR_SEND_PENDING);
    }

    void enable_intr(uint8_t const spurious_vector)
    {
      regs->spurious_vector.reg = 0x100 | spurious_vector;
    }

    apic_regs* regs;
  };
  extern apic lapic;
}

#endif
