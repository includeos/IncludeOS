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
#include <hw/xapic.hpp>
#include <hw/x2apic.hpp>
#include <hw/ioapic.hpp>
#include <hw/acpi.hpp> // ACPI
#include <hw/cpu.hpp>
#include <hw/pic.hpp>
#include <hw/smp.hpp>
#include <kernel/cpuid.hpp>
#include <kernel/irq_manager.hpp>
#include <cstdio>
#include <stdlib.h>
#include <debug>
#include <kprint>
#include <info>

extern "C" {
  // current selected EOI method
  void (*current_eoi_mechanism)();
  // KVM para PV-EOI feature
  void kvm_pv_eoi();
  void kvm_pv_eoi_init();
  // easier deduction of type
  void x2apic_send_eoi() {
    hw::x2apic::get().eoi();
  }
}

namespace hw
{
  static IApic* current_apic;
  IApic& APIC::get() noexcept {
    return *current_apic;
  }
  
  void APIC::init()
  {
    // disable the legacy 8259 PIC
    // by masking off all interrupts
    hw::PIC::set_intr_mask(0xFFFF);

    // a PC without APIC is insane
    assert(CPUID::has_feature(CPUID::Feature::APIC) && "If this fails, the machine is insane");

    if (CPUID::has_feature(CPUID::Feature::X2APIC)) {
        current_apic = new x2apic();
        current_eoi_mechanism = x2apic_send_eoi;
    } else {
        current_apic = new xapic();
        current_eoi_mechanism = lapic_send_eoi;
    }

    // enable xAPIC/x2APIC on this cpu
    current_apic->enable();

    // initialize I/O APICs
    IOAPIC::init(ACPI::get_ioapics());

    /// initialize and start APs found in ACPI-tables ///
    if (ACPI::get_cpus().size() > 1) {
      INFO("APIC", "SMP Init");
      // initialize and start registered APs found in ACPI-tables
      SMP::init();
      // IRQ handler for completed async jobs
      IRQ_manager::get().subscribe(BSP_LAPIC_IPI_IRQ,
      [] {
        // copy all the done functions out from queue to our local vector
        auto done = SMP::get_completed();
        // call all the done functions
        for (auto& func : done) {
          func();
        }
      });
    }

    // use KVMs paravirt EOI if supported
    //if (CPUID::kvm_feature(KVM_FEATURE_PV_EOI))
    //    kvm_pv_eoi_init();
  }

  void APIC::enable_irq(uint8_t irq)
  {
    auto& overrides = ACPI::get_overrides();
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
    auto& overrides = ACPI::get_overrides();
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

#define _ADDR_ (*(volatile long *) addr)
int __test_and_clear_bit(long nr, volatile unsigned long* addr)
{
  int oldbit;

	asm volatile( "lock "
		"btrl %2,%1\n\tsbbl %0,%0"
		:"=r" (oldbit),"=m" (_ADDR_)
		:"dIr" (nr) : "memory");
	return oldbit;
}

__attribute__ ((aligned(4)))
static volatile unsigned long kvm_apic_eoi = KVM_PV_EOI_DISABLED;
void kvm_pv_eoi()
{
  kprintf("BEFOR: %#lx  intr %u  irr %u\n", kvm_apic_eoi, hw::APIC::get_isr(), hw::APIC::get_irr());
  // fast EOI by KVM
  if (__test_and_clear_bit(KVM_PV_EOI_BIT, &kvm_apic_eoi)) {
      kprintf("avoided\n");
      return;
  }
  // fallback to normal APIC EOI
  lapic_send_eoi();
  // check after
  kprintf("AFTER: %#lx  intr %u  irr %u\n", kvm_apic_eoi, hw::APIC::get_isr(), hw::APIC::get_irr());
}
void kvm_pv_eoi_init()
{
  kprintf("* Enabling KVM paravirtual EOI\n");
  // set new EOI handler
  current_eoi_mechanism = kvm_pv_eoi;
  kvm_apic_eoi = 0;
  union {
    uint32_t msr[2];
    uint64_t whole;
  } guest;
  guest.whole = (uint64_t) &kvm_apic_eoi;
  guest.whole |= KVM_MSR_ENABLED;
  
  kprintf("  MSR %#x  addr = %#llx\n", MSR_KVM_PV_EOI_EN, guest.whole);
  hw::CPU::write_msr(MSR_KVM_PV_EOI_EN, guest.msr[0], guest.msr[1]);
  // verify that the feature was enabled
  uint64_t res = hw::CPU::read_msr(MSR_KVM_PV_EOI_EN);
  assert(res == guest.whole);
}
