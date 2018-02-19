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
#include <kernel/events.hpp>
#include <kprint>
#include <info>
//#define ENABLE_KVM_PV_EOI
//#define ENABLE_DYNAMIC_EOI

namespace x86
{
  static IApic* current_apic = nullptr;
  IApic& APIC::get() noexcept {
    return *current_apic;
  }
}

extern "C" {
  // current selected EOI method
  void (*current_eoi_mechanism)() = nullptr;
  void (*real_eoi_mechanism)() = nullptr;
  void (*current_intr_handler)()  = nullptr;
  // shortcut that avoids virtual call
  void x2apic_send_eoi() {
    x86::CPU::write_msr(x86::x2apic::BASE_MSR + x2APIC_EOI, 0);
  }
  // the various interrupt handler flavors
  void xapic_intr_handler()
  {
    uint8_t vector = x86::APIC::get_isr();
    //assert(vector >= IRQ_BASE && vector < 160);
    Events::get().trigger_event(vector - IRQ_BASE);
#ifdef ENABLE_DYNAMIC_EOI
    assert(current_eoi_mechanism != nullptr);
    current_eoi_mechanism();
#else
    lapic_send_eoi();
#endif
  }
  void x2apic_intr_handler()
  {
    uint8_t vector = x86::x2apic::static_get_isr();
    //assert(vector >= IRQ_BASE && vector < 160);
    Events::get().trigger_event(vector - IRQ_BASE);
#ifdef ENABLE_DYNAMIC_EOI
    assert(current_eoi_mechanism != nullptr);
    current_eoi_mechanism();
#else
    x2apic_send_eoi();
#endif
  }
}

extern void kvm_pv_eoi_init();

namespace x86
{
  void APIC::init()
  {
    // disable the legacy 8259 PIC
    // by masking off all interrupts
    PIC::init();
    PIC::disable();

    if (CPUID::has_feature(CPUID::Feature::X2APIC)) {
        current_apic = &x2apic::get();
        real_eoi_mechanism = x2apic_send_eoi;
        current_intr_handler  = x2apic_intr_handler;
    } else {
        // an x86 PC without APIC is insane
        assert(CPUID::has_feature(CPUID::Feature::APIC)
            && "If this fails, the machine is insane");
        current_apic = &xapic::get();
        real_eoi_mechanism = lapic_send_eoi;
        current_intr_handler  = xapic_intr_handler;
    }

    if (current_eoi_mechanism == nullptr)
        current_eoi_mechanism = real_eoi_mechanism;

    // enable xAPIC/x2APIC on this cpu
    current_apic->enable();

    // initialize I/O APICs
    IOAPIC::init(ACPI::get_ioapics());

#ifdef ENABLE_KVM_PV_EOI
    // use KVMs paravirt EOI if supported
    if (CPUID::kvm_feature(KVM_FEATURE_PV_EOI))
        kvm_pv_eoi_init();
#endif
  }

  void APIC::enable_irq(uint8_t irq)
  {
    auto& overrides = x86::ACPI::get_overrides();
    for (auto& redir : overrides)
    {
      // NOTE: @bus_source is the IOAPIC number
      if (redir.irq_source == irq)
      {
        if (OS::is_panicking() == false)
        {
          INFO2("Enabled redirected entry %u ioapic %u -> %u on apic %u",
              redir.global_intr, redir.bus_source, irq, get().get_id());
        }
        IOAPIC::enable(get().get_id(), redir);
        return;
      }
    }
    if (OS::is_panicking() == false)
    {
      INFO2("Enabled non-redirected IRQ %u on apic %u", irq, get().get_id());
    }
    IOAPIC::enable(get().get_id(), {0, 0, 0, irq, irq, 0});
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
