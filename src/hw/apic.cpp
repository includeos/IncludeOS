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
#include <hw/apic_regs.hpp>
#include <hw/ioapic.hpp>
#include <hw/acpi.hpp> // ACPI
#include <hw/apic_revenant.hpp>
#include <hw/cpu.hpp>
#include <hw/pic.hpp>
#include <kernel/irq_manager.hpp>
#include <cstdio>
#include <debug>
#include <info>

extern "C" {
  void apic_enable();
  int  get_cpu_id();
  extern char _binary_apic_boot_bin_start;
  extern char _binary_apic_boot_bin_end;
  void lapic_send_eoi();
  void lapic_exception_handler();
  void lapic_irq_entry();
  // current selected EOI method
  void (*current_eoi_mechanism)() = lapic_send_eoi;
  // KVM para PV-EOI feature
  void kvm_pv_eoi();
  void kvm_pv_eoi_init();
}
extern idt_loc smp_lapic_idt;

namespace hw {

  static const uintptr_t IA32_APIC_BASE_MSR = 0x1B;
  static const uintptr_t IA32_APIC_BASE_MSR_ENABLE = 0x800;
  static const uintptr_t BOOTLOADER_LOCATION = 0x80000;
  static const uint8_t   SPURIOUS_INTR = IRQ_manager::INTR_LINES-1;

  // every CPU has a local APIC
  apic lapic;

  struct apic_boot {
    // the jump instruction at the start
    uint32_t   jump;
    // stuff we will need to modify
    void*  worker_addr;
    void*  stack_base;
    size_t stack_size;
  };

  union addr_union {
    uint32_t whole;
    uint16_t part[2];

    addr_union(void(*addr)()) {
      whole = (uintptr_t) addr;
    }
  };

  void APIC::init()
  {
    const uint64_t APIC_BASE_MSR = CPU::read_msr(IA32_APIC_BASE_MSR);
    /// find the LAPICs base address ///
    const uintptr_t APIC_BASE_ADDR = APIC_BASE_MSR & 0xFFFFF000;
    // acquire infos
    lapic = apic(APIC_BASE_ADDR);
    INFO2("LAPIC id: %x  ver: %x\n", lapic.get_id(), lapic.regs->lapic_ver.reg);

    // disable the legacy 8259 PIC
    // by masking off all interrupts
    hw::PIC::set_intr_mask(0xFFFF);

    // enable Local APIC
    void _lapic_enable();
    _lapic_enable();

    // turn the Local APIC on
    INFO("APIC", "Enabling BSP LAPIC");
    CPU::write_msr(IA32_APIC_BASE_MSR,
        (APIC_BASE_MSR & 0xfffff100) | IA32_APIC_BASE_MSR_ENABLE, 0);
    INFO2("APIC_BASE MSR is now 0x%llx\n", CPU::read_msr(IA32_APIC_BASE_MSR));

    // initialize I/O APICs
    IOAPIC::init(ACPI::get_ioapics());

    /// initialize and start APs found in ACPI-tables ///
    if (ACPI::get_cpus().size() > 1) {
      INFO("APIC", "SMP Init");
      init_smp();
    }

    // use KVMs paravirt EOI if supported
    //kvm_pv_eoi_init();

    // subscribe to APIC-related interrupts
    setup_subs();
  }

  void _lapic_enable()
  {
    /// enable interrupts ///
    lapic.regs->task_pri.reg       = 0xff;
    lapic.regs->dest_format.reg    = 0xffffffff; // flat mode
    lapic.regs->logical_dest.reg   = 0x01000000; // logical ID 1

    // program local interrupts
    lapic.regs->lint0.reg = INTR_MASK | LAPIC_IRQ_LINT0;
    lapic.regs->lint1.reg = INTR_MASK | LAPIC_IRQ_LINT1;
    lapic.regs->error.reg = INTR_MASK | LAPIC_IRQ_ERROR;

    // start receiving interrupts (0x100), set spurious vector
    // note: spurious IRQ must have 4 last bits set (0x?F)
    lapic.enable_intr(SPURIOUS_INTR);

    // acknowledge any outstanding interrupts
    (*current_eoi_mechanism)();

    // enable APIC by resetting task priority
    lapic.regs->task_pri.reg = 0;
  }

  /// initialize and start registered APs found in ACPI-tables ///
  void APIC::init_smp()
  {
    // smp with only one CPU == :facepalm:
    assert(ACPI::get_cpus().size() > 1);

    // copy our bootloader to APIC init location
    const char* start = &_binary_apic_boot_bin_start;
    ptrdiff_t bootloader_size = &_binary_apic_boot_bin_end - start;
    debug("Copying bootloader from %p to 0x%x (size=%d)\n",
          start, BOOTLOADER_LOCATION, bootloader_size);
    memcpy((char*) BOOTLOADER_LOCATION, start, bootloader_size);

    // modify bootloader to support our cause
    auto* boot = (apic_boot*) BOOTLOADER_LOCATION;
    // populate IDT used with SMP LAPICs
    smp_lapic_idt.limit = 256 * sizeof(IDTDescr) - 1;
    smp_lapic_idt.base  = (uintptr_t) new IDTDescr[256];

    auto* idt = (IDTDescr*) smp_lapic_idt.base;
    for (size_t i = 0; i < 32; i++) {
      addr_union addr(lapic_exception_handler);
      idt[i].offset_1 = addr.part[0];
      idt[i].offset_2 = addr.part[1];
      idt[i].selector  = 0x8;
      idt[i].type_attr = 0x8e;
      idt[i].zero      = 0;
    }
    for (size_t i = 32; i < 48; i++) {
      addr_union addr(lapic_irq_entry);
      idt[i].offset_1 = addr.part[0];
      idt[i].offset_2 = addr.part[1];
      idt[i].selector  = 0x8;
      idt[i].type_attr = 0x8e;
      idt[i].zero      = 0;
    }

    // assign stack and main func
    size_t CPUcount = ACPI::get_cpus().size();

    boot->worker_addr = (void*) &revenant_main;
    boot->stack_base = aligned_alloc(CPUcount * REV_STACK_SIZE, 4096);
    boot->stack_size = REV_STACK_SIZE;
    debug("APIC stack base: %p  size: %u   main size: %u\n",
        boot->stack_base, boot->stack_size, sizeof(boot->worker_addr));

    // reset barrier
    smp.boot_barrier.reset(1);

    // turn on CPUs
    INFO("APIC", "Initializing APs");
    for (auto& cpu : ACPI::get_cpus())
    {
      debug("-> CPU %u ID %u  fl 0x%x\n",
        cpu.cpu, cpu.id, cpu.flags);
      // except the CPU we are using now
      if (cpu.id != lapic.get_id())
        lapic.ap_init(cpu.id);
    }
    // start CPUs
    INFO("APIC", "Starting APs");
    for (auto& cpu : ACPI::get_cpus())
    {
      // except the CPU we are using now
      if (cpu.id == lapic.get_id()) continue;
      // Send SIPI with start address 0x80000
      lapic.ap_start(cpu.id, 0x80);
      lapic.ap_start(cpu.id, 0x80);
    }

    // wait for all APs to start
    smp.boot_barrier.spin_wait(CPUcount);
    INFO("APIC", "All APs are online now\n");
  }

  uint8_t APIC::get_isr()
  {
    for (uint8_t i = 0; i < 8; i++)
      if (lapic.regs->isr[i].reg)
        return 32 * i + __builtin_ffs(lapic.regs->isr[i].reg) - 1;
    return 255;
  }
  uint8_t APIC::get_irr()
  {
    for (uint8_t i = 0; i < 8; i++)
      if (lapic.regs->irr[i].reg)
        return 32 * i + __builtin_ffs(lapic.regs->irr[i].reg) - 1;
    return 255;
  }

  void APIC::eoi()
  {
    debug("-> eoi @ %p for %u\n", &lapic.regs->eoi.reg, lapic.get_id());
    lapic.regs->eoi.reg = 0;
  }

  void APIC::send_ipi(uint8_t id, uint8_t vector)
  {
    debug("send_ipi  id %u  vector %u\n", id, vector);
    // select APIC ID
    uint32_t value = lapic.regs->intr_hi.reg & 0x00ffffff;
    lapic.regs->intr_hi.reg = value | (id << 24);
    // write vector and trigger/level/mode
    value = ICR_ASSERT | ICR_FIXED | vector;
    lapic.regs->intr_lo.reg = value;
  }
  void APIC::send_bsp_intr()
  {
    // for now we will just assume BSP is 0
    send_ipi(0, 32 + BSP_LAPIC_IPI_IRQ);
  }
  void APIC::bcast_ipi(uint8_t vector)
  {
    debug("bcast_ipi  vector %u\n", vector);
    //lapic.regs->intr_hi.reg = id << 24;
    lapic.regs->intr_lo.reg = ICR_ALL_EXCLUDING_SELF | ICR_ASSERT | vector;
  }

  void APIC::add_task(smp_task_func task, smp_done_func done)
  {
    lock(smp.tlock);
    smp.tasks.emplace_back(task, done);
    unlock(smp.tlock);
  }
  void APIC::work_signal()
  {
    // broadcast that we have work to do
    bcast_ipi(0x20);
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
            redir.irq_source, redir.global_intr, lapic.get_id());
        IOAPIC::enable(redir.global_intr, irq, lapic.get_id());
        return;
      }
    }
    INFO2("Enabled non-redirected IRQ %u on LAPIC %u", irq, lapic.get_id());
    IOAPIC::enable(irq, irq, lapic.get_id());
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
  void APIC::setup_subs()
  {
    // IRQ handler for completed async jobs
    IRQ_manager::get().subscribe(BSP_LAPIC_IPI_IRQ,
    [] {
      // copy all the done functions out from queue to our local vector
      std::vector<smp_done_func> done;
      lock(smp.flock);
      for (auto& func : smp.completed)
        done.push_back(func);
      unlock(smp.flock);

      // call all the done functions
      for (auto& func : done) {
        func();
      }
    });
  }
}

// *** manual ***
// http://choon.net/forum/read.php?21,1123399
// https://www.kernel.org/doc/Documentation/virtual/kvm/cpuid.txt

#define KVM_FEATURE_PV_EOI     6
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

static volatile unsigned long kvm_apic_eoi = KVM_PV_EOI_DISABLED;
void kvm_pv_eoi() {

  //printf("BEFOR: %#lx  intr %u  irr %u\n", kvm_apic_eoi, hw::APIC::get_isr(), hw::APIC::get_irr());
  // fast EOI by KVM
  if (__test_and_clear_bit(KVM_PV_EOI_BIT, &kvm_apic_eoi)) {
      printf("avoided\n");
      return;
  }
  // fallback to normal APIC EOI
  hw::lapic.regs->eoi.reg = 0;
  // check after
  //printf("AFTER: %#lx  intr %u  irr %u\n", kvm_apic_eoi, hw::APIC::get_isr(), hw::APIC::get_irr());
}
void kvm_pv_eoi_init() {
  kvm_apic_eoi = 0;
  auto pv_eoi = (uintptr_t) &kvm_apic_eoi;
  printf("MSR %#x  pv_eoi = %#x\n", MSR_KVM_PV_EOI_EN, pv_eoi);
  hw::CPU::write_msr(MSR_KVM_PV_EOI_EN, pv_eoi | KVM_MSR_ENABLED, 0);
  // set new EOI handler
  current_eoi_mechanism = kvm_pv_eoi;
  kvm_apic_eoi = KVM_PV_EOI_ENABLED;
}
