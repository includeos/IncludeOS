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
#ifndef X86_X2APIC_HPP
#define X86_X2APIC_HPP

#include "apic_iface.hpp"
#include "apic_regs.hpp"
#include <arch/x86/cpu.hpp>
#include <kernel/events.hpp>
#include <debug>
#include <info>

/// x2APIC MSR offsets ///
#define x2APIC_ID         0x02
#define x2APIC_VER        0x03
#define x2APIC_TPR        0x08
#define x2APIC_EOI        0x0B
#define x2APIC_LDR        0x0D
#define x2APIC_DFR        0x0E
#define x2APIC_SPURIOUS   0x0F
#define x2APIC_ISR        0x10
#define x2APIC_IRR        0x20
#define x2APIC_ESR        0x28
#define x2APIC_ICR        0x30
#define x2APIC_LVT_TMR    0x32
#define x2APIC_LVT_PERF   0x34
#define x2APIC_LVT_LINT0  0x35
#define x2APIC_LVT_LINT1  0x36
#define x2APIC_LVT_ERR    0x37
#define x2APIC_TMRINITCNT 0x38
#define x2APIC_TMRCURRCNT 0x39
#define x2APIC_TMRDIV     0x3E

#define IA32_APIC_BASE_MSR  0x1B

namespace x86 {

  struct x2apic : public IApic
  {
    static const uint32_t MSR_ENABLE_X2APIC = 0xC00;
    static const uint8_t  SPURIOUS_INTR = IRQ_BASE + Events::NUM_EVENTS-1;

    x2apic() {
      auto base_msr = CPU::read_msr(IA32_APIC_BASE_MSR);
      INFO("x2APIC", "Enabling x2APIC @ %#x", (uint32_t) base_msr);

      // add x2APIC enable bit to APIC BASE MSR
      base_msr = (base_msr & 0xfffff000) | MSR_ENABLE_X2APIC;
      // turn the x2APIC on
      CPU::write_msr(IA32_APIC_BASE_MSR, base_msr, 0);
      // verify that x2APIC is online
      uint64_t verify = CPU::read_msr(IA32_APIC_BASE_MSR);
      assert(verify & MSR_ENABLE_X2APIC);
      INFO2("APIC id: %x  ver: %x", get_id(), version());
    }

    uint32_t read(uint32_t reg) noexcept
    {
      return CPU::read_msr(BASE_MSR + reg);
    }
    uint64_t readl(uint32_t reg) noexcept
    {
      return CPU::read_msr(BASE_MSR + reg);
    }
    void write(uint32_t reg, uint32_t value) noexcept
    {
      CPU::write_msr(BASE_MSR + reg, value);
    }
    void writel(uint32_t reg, uint32_t hi, uint32_t lo) noexcept
    {
      CPU::write_msr(BASE_MSR + reg, lo, hi);
    }

    const char* name() const noexcept override {
      return "x2APIC";
    }
    uint32_t get_id() noexcept override {
      return read(x2APIC_ID);
    }
    uint32_t version() noexcept override {
      return read(x2APIC_VER);
    }

    void
    interrupt_control(uint32_t bits, uint8_t spurious) noexcept override
    {
      write(x2APIC_SPURIOUS, bits | spurious);
    }

    void enable() noexcept override
    {
      write(x2APIC_TPR, 0xff);       // disable temp

      // program APIC local interrupts
      write(x2APIC_LVT_LINT0, INTR_MASK | LAPIC_IRQ_LINT0);
      write(x2APIC_LVT_LINT1, INTR_MASK | LAPIC_IRQ_LINT1);
      write(x2APIC_LVT_ERR,   INTR_MASK | LAPIC_IRQ_ERROR);

      // start receiving interrupts (0x100), set spurious vector
      // note: spurious IRQ must have 4 last bits set (0x?F)
      interrupt_control(0x100, SPURIOUS_INTR);

      // acknowledge any outstanding interrupts
      this->eoi();

      // enable APIC by resetting task priority
      write(x2APIC_TPR, 0);
    }
    void smp_enable() noexcept override {
      // enable x2apic
      auto base_msr = CPU::read_msr(IA32_APIC_BASE_MSR);
      base_msr = (base_msr & 0xfffff100) | MSR_ENABLE_X2APIC;
      CPU::write_msr(IA32_APIC_BASE_MSR, base_msr, 0);
      // configure it
      enable();
    }

    static int static_get_isr() noexcept
    {
      for (int i = 5; i >= 1; i--) {
        uint32_t reg = CPU::read_msr(BASE_MSR + x2APIC_ISR + i);
        if (reg) return 32 * i + __builtin_ffs(reg) - 1;
      }
      return -1;
    }

    void eoi() noexcept override
    {
      write(x2APIC_EOI, 0);
    }
    int get_isr() noexcept override
    {
      for (int i = 5; i >= 1; i--) {
        uint32_t reg = read(x2APIC_ISR + i);
        if (reg) return 32 * i + __builtin_ffs(reg) - 1;
      }
      return -1;
    }
    int get_irr() noexcept override
    {
      for (int i = 5; i >= 0; i--) {
        uint32_t reg = read(x2APIC_IRR + i);
        if (reg) return 32 * i + __builtin_ffs(reg) - 1;
      }
      return -1;
    }
    static uint32_t get_isr_at(int index) noexcept
    {
      return CPU::read_msr(BASE_MSR + x2APIC_ISR + index);
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
      writel(x2APIC_ICR, id,
          ICR_INIT | ICR_PHYSICAL | ICR_ASSERT | ICR_EDGE | ICR_NO_SHORTHAND);
      while (readl(x2APIC_ICR) & ICR_SEND_PENDING);
    }
    void ap_start(int id, uint32_t vector) noexcept override
    {
      writel(x2APIC_ICR, id,
          vector | ICR_STARTUP | ICR_PHYSICAL | ICR_ASSERT | ICR_EDGE | ICR_NO_SHORTHAND);
      while (readl(x2APIC_ICR) & ICR_SEND_PENDING);
    }

    void send_ipi(int id, uint8_t vector) noexcept override
    {
      debug("send_ipi  id %u  vector %u\n", id, vector);
      // select APIC ID
      writel(x2APIC_ICR, id,
                         ICR_ASSERT | ICR_FIXED | vector);
    }
    void send_bsp_intr() noexcept override
    {
      // for now we will just assume BSP is 0
      send_ipi(0, 32 + BSP_LAPIC_IPI_IRQ);
    }
    void bcast_ipi(uint8_t vector) noexcept override
    {
      debug("bcast_ipi  vector %u\n", vector);
      writel(x2APIC_ICR, 0,
                         ICR_ALL_EXCLUDING_SELF | ICR_ASSERT | vector);
    }

    void timer_init(const uint8_t timer_intr) noexcept override
    {
      static const uint32_t TIMER_ONESHOT = 0x0;
      // decrement every other tick
      write(x2APIC_TMRDIV, 0x1);
      // start in one-shot mode and set the interrupt vector
      // but also disable interrupts
      write(x2APIC_LVT_TMR, TIMER_ONESHOT | (32+timer_intr) | INTR_MASK);
    }
    void timer_begin(uint32_t value) noexcept override
    {
      write(x2APIC_TMRINITCNT, value);
    }
    uint32_t timer_diff() noexcept override
    {
      return read(x2APIC_TMRINITCNT)-read(x2APIC_TMRCURRCNT);
    }
    void timer_interrupt(bool enabled) noexcept override
    {
      if (enabled)
          write(x2APIC_LVT_TMR, read(x2APIC_LVT_TMR) & ~INTR_MASK);
      else
          write(x2APIC_LVT_TMR, read(x2APIC_LVT_TMR) | INTR_MASK);
    }

    static x2apic& get() {
      static x2apic instance;
      return instance;
    }

    static const uint32_t BASE_MSR = 0x800;
  };
}

#endif
