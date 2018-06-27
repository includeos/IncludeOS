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
#ifndef X86_XAPIC_HPP
#define X86_XAPIC_HPP

#include "apic_iface.hpp"
#include "apic_regs.hpp"
#include <arch/x86/cpu.hpp>
#include <kernel/events.hpp>
#include <debug>
#include <info>

/// xAPIC MMIO offsets ///
#define xAPIC_ID         0x20
#define xAPIC_VER        0x30
#define xAPIC_TPR        0x80
#define xAPIC_EOI        0x0B0
#define xAPIC_LDR        0x0D0
#define xAPIC_DFR        0x0E0
#define xAPIC_SPURIOUS   0x0F0
#define xAPIC_ISR        0x100
#define xAPIC_IRR        0x200
#define xAPIC_ESR        0x280
#define xAPIC_ICRL       0x300
#define xAPIC_ICRH       0x310
#define xAPIC_LVT_TMR    0x320
#define xAPIC_LVT_PERF   0x340
#define xAPIC_LVT_LINT0  0x350
#define xAPIC_LVT_LINT1  0x360
#define xAPIC_LVT_ERR    0x370
#define xAPIC_TMRINITCNT 0x380
#define xAPIC_TMRCURRCNT 0x390
#define xAPIC_TMRDIV     0x3E0
#define xAPIC_LAST       0x38F

#define IA32_APIC_BASE_MSR  0x1B

extern "C" {
  extern void (*current_eoi_mechanism)();
  extern void lapic_send_eoi();
}

namespace x86 {

  struct xapic : public IApic
  {
    static const uint32_t MSR_ENABLE_XAPIC   = 0x800;
    static const uint8_t  SPURIOUS_INTR = IRQ_BASE + Events::NUM_EVENTS-1;

    xapic() {
      // read xAPIC base address from masked out MSR
      this->base_msr = CPU::read_msr(IA32_APIC_BASE_MSR);
      this->regbase = this->base_msr & 0xFFFFF000;
      // turn the xAPIC on
      INFO("xAPIC", "Enabling xAPIC");
      this->base_msr =
          (this->base_msr & 0xfffff100) | MSR_ENABLE_XAPIC;
      CPU::write_msr(IA32_APIC_BASE_MSR, this->base_msr);
      // verify that xAPIC is online
      uint64_t verify = CPU::read_msr(IA32_APIC_BASE_MSR);
      assert(verify & MSR_ENABLE_XAPIC);
      INFO2("ID: %x  Ver: %x", get_id(), version());
    }

    uint32_t read(uint32_t reg) noexcept
    {
      return *(volatile uint32_t*) (regbase + reg);
    }
    void write(uint32_t reg, uint32_t value) noexcept
    {
      *(volatile uint32_t*) (regbase + reg) = value;
    }

    const char* name() const noexcept override {
      return "xAPIC";
    }
    uint32_t get_id() noexcept override {
      return read(xAPIC_ID) >> 24;
    }
    uint32_t version() noexcept override {
      return read(xAPIC_VER);
    }

    void
    interrupt_control(uint32_t bits, uint8_t spurious) noexcept override
    {
      write(xAPIC_SPURIOUS, bits | spurious);
    }

    void enable() noexcept override
    {
      /// enable interrupts ///
      write(xAPIC_TPR, 0xff);       // disable temp
      write(xAPIC_LDR, 0x01000000); // logical ID 1
      write(xAPIC_DFR, 0xffffffff); // flat destination mode

      // program APIC local interrupts
      write(xAPIC_LVT_LINT0, INTR_MASK | LAPIC_IRQ_LINT0);
      write(xAPIC_LVT_LINT1, INTR_MASK | LAPIC_IRQ_LINT1);
      write(xAPIC_LVT_ERR,   INTR_MASK | LAPIC_IRQ_ERROR);

      // start receiving interrupts (0x100), set spurious vector
      // note: spurious IRQ must have 4 last bits set (0x?F)
      interrupt_control(0x100, SPURIOUS_INTR);

      // acknowledge any outstanding interrupts
      lapic_send_eoi();

      // enable APIC by resetting task priority
      write(xAPIC_TPR, 0);
    }
    void smp_enable() noexcept override {
      enable();
    }

    void eoi() noexcept override
    {
      write(xAPIC_EOI, 0);
    }
    int get_isr() noexcept override
    {
      for (int i = 5; i >= 1; i--) {
        uint32_t reg = read(xAPIC_ISR + 0x10 * i);
        if (reg) return 32 * i + __builtin_ffs(reg) - 1;
      }
      return -1;
    }
    int get_irr() noexcept override
    {
      for (int i = 5; i >= 0; i--) {
        uint32_t reg = read(xAPIC_IRR + 0x10 * i);
        if (reg) return 32 * i + __builtin_ffs(reg) - 1;
      }
      return -1;
    }
    static uint32_t get_isr_at(int index) noexcept
    {
      return *(volatile uint32_t*) uintptr_t(0xfee00000 + xAPIC_ISR + 0x10 * index);
    }
    static std::array<uint32_t, 6> get_isr_array() noexcept
    {
      std::array<uint32_t, 6> isr_array;
      for (int i = 1; i < 6; i++) {
        isr_array[i] = get_isr_at(i);
      }
      return isr_array;
    }

    void ap_init(int id) noexcept override
    {
      write(xAPIC_ICRH, (id & 0xff) << ICR_DEST_BITS);
      write(xAPIC_ICRL, ICR_INIT | ICR_PHYSICAL
           | ICR_ASSERT | ICR_EDGE | ICR_NO_SHORTHAND);
      while (read(xAPIC_ICRL) & ICR_SEND_PENDING);
    }
    void ap_start(int id, uint32_t vector) noexcept override
    {
      write(xAPIC_ICRH, (id & 0xff) << ICR_DEST_BITS);
      write(xAPIC_ICRL, vector | ICR_STARTUP
        | ICR_PHYSICAL | ICR_ASSERT | ICR_EDGE | ICR_NO_SHORTHAND);
      while (read(xAPIC_ICRL) & ICR_SEND_PENDING);
    }

    void send_ipi(int id, uint8_t vector) noexcept override
    {
      debug("send_ipi  id %u  vector %u\n", id, vector);
      // select APIC ID
      uint32_t value = read(xAPIC_ICRH) & 0x00ffffff;
      write(xAPIC_ICRH, value | ((id & 0xff) << 24));
      // write vector and trigger/level/mode
      write(xAPIC_ICRL, ICR_ASSERT | ICR_FIXED | vector);
    }
    void send_bsp_intr() noexcept override
    {
      // for now we will just assume BSP is 0
      send_ipi(0, 32 + BSP_LAPIC_IPI_IRQ);
    }
    void bcast_ipi(uint8_t vector) noexcept override
    {
      debug("bcast_ipi  vector %u\n", vector);
      write(xAPIC_ICRL, ICR_ALL_EXCLUDING_SELF | ICR_ASSERT | vector);
    }

    void timer_init(const uint8_t timer_intr) noexcept override
    {
      static const uint32_t TIMER_ONESHOT = 0x0;
      // decrement every other tick
      write(xAPIC_TMRDIV, 0x1);
      // start in one-shot mode and set the interrupt vector
      // but also disable interrupts
      write(xAPIC_LVT_TMR, TIMER_ONESHOT | (32+timer_intr) | INTR_MASK);
    }
    void timer_begin(uint32_t value) noexcept override
    {
      write(xAPIC_TMRINITCNT, value);
    }
    uint32_t timer_diff() noexcept override
    {
      return read(xAPIC_TMRINITCNT) - read(xAPIC_TMRCURRCNT);
    }
    void timer_interrupt(bool enabled) noexcept override
    {
      if (enabled)
          write(xAPIC_LVT_TMR, read(xAPIC_LVT_TMR) & ~INTR_MASK);
      else
          write(xAPIC_LVT_TMR, read(xAPIC_LVT_TMR) | INTR_MASK);
    }

    static xapic& get() {
      static xapic instance;
      return instance;
    }
  private:
    uint64_t  base_msr;
    uintptr_t regbase;
  };
}

#endif
