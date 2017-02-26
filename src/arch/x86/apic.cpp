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

#include "apic.hpp"
#include "ioapic.hpp"
#include "acpi.hpp"
#include "cpu.hpp"
#include "pic.hpp"
#include "smp.hpp"
#include <kernel/cpuid.hpp>
#include <kernel/irq_manager.hpp>
#include <cstdlib>
#include <debug>
#include <kprint>
#include <info>

extern "C" {
  // current selected EOI method
  void (*current_eoi_mechanism)();
  // KVM para PV-EOI feature
  void kvm_pv_eoi();
  // shortcut that avoids virtual call
  void x2apic_send_eoi() {
    x86::CPU::write_msr(x86::x2apic::BASE_MSR + x2APIC_EOI, 0);
  }
}

void kvm_pv_eoi_init();

namespace x86
{
  static IApic* current_apic = nullptr;
  IApic& APIC::get() noexcept {
    return *current_apic;
  }
  
  void APIC::init()
  {
    // disable the legacy 8259 PIC
    // by masking off all interrupts
    PIC::set_intr_mask(0xFFFF);

    if (CPUID::has_feature(CPUID::Feature::X2APIC)) {
        current_apic = &x2apic::get();
        current_eoi_mechanism = x2apic_send_eoi;
    } else {
        // an x86 PC without APIC is insane
        assert(CPUID::has_feature(CPUID::Feature::APIC) 
            && "If this fails, the machine is insane");
        current_apic = &xapic::get();
        current_eoi_mechanism = lapic_send_eoi;
    }

    // enable xAPIC/x2APIC on this cpu
    current_apic->enable();

    // initialize I/O APICs
    IOAPIC::init(ACPI::get_ioapics());

    // use KVMs paravirt EOI if supported
    //if (CPUID::kvm_feature(KVM_FEATURE_PV_EOI))
    //    kvm_pv_eoi_init();
  }

  void APIC::enable_irq(uint8_t irq)
  {
    auto& overrides = x86::ACPI::get_overrides();
    for (auto& redir : overrides)
    {
      // NOTE: @bus_source is the IOAPIC number
      if (redir.irq_source == irq)
      {
        INFO2("Enabled redirected IRQ %u -> %u on lapic %u",
            redir.irq_source, redir.global_intr, get().get_id());
        IOAPIC::enable(redir.global_intr, irq, get().get_id());
        return;
      }
    }
    INFO2("Enabled non-redirected IRQ %u on LAPIC %u", irq, get().get_id());
    IOAPIC::enable(irq, irq, get().get_id());
  }
  void APIC::disable_irq(uint8_t irq)
  {
    auto& overrides = x86::ACPI::get_overrides();
    for (auto& redir : overrides)
    {
      // NOTE: @bus_source is the IOAPIC number
      if (redir.irq_source == irq)
      {
        IOAPIC::disable(redir.global_intr);
        return;
      }
    }
    IOAPIC::disable(irq);
  }
}

// *** manual ***
// http://choon.net/forum/read.php?21,1123399
// https://www.kernel.org/doc/Documentation/virtual/kvm/cpuid.txt

#define KVM_MSR_ENABLED        1
#define MSR_KVM_PV_EOI_EN      0x4b564d04
#define KVM_PV_EOI_BIT         0
#define KVM_PV_EOI_MASK       (0x1 << KVM_PV_EOI_BIT)
#define KVM_PV_EOI_ENABLED     KVM_PV_EOI_MASK
#define KVM_PV_EOI_DISABLED    0x0

__attribute__ ((aligned(4)))
static volatile unsigned long kvm_exitless_eoi = KVM_PV_EOI_DISABLED;

void kvm_pv_eoi()
{
  uint8_t reg;
  asm("btr %2, %0; setc %1" : "+m"(kvm_exitless_eoi), "=rm"(reg) : "r"(0));
  if (reg) {
      kprintf("avoided\n");
      return;
  }
  // fallback to normal x2APIC EOI
  x2apic_send_eoi();
}
void kvm_pv_eoi_init()
{
  union {
    uint32_t msr[2];
    uint64_t whole;
  } guest;
  guest.whole = (uint64_t) &kvm_exitless_eoi;
  guest.whole |= KVM_MSR_ENABLED;
  x86::CPU::write_msr(MSR_KVM_PV_EOI_EN, guest.msr[0], guest.msr[1]);
  // verify that the feature was enabled
  uint64_t res = x86::CPU::read_msr(MSR_KVM_PV_EOI_EN);
  if (res & 1) {
    kprintf("* KVM paravirtual EOI enabled\n");
    // set new EOI handler
    current_eoi_mechanism = kvm_pv_eoi;
  }
}
