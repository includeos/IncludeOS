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
#include <hw/ioapic.hpp>
#include <hw/acpi.hpp> // ACPI
#include <hw/apic_revenant.hpp>
#include <hw/ioport.hpp>
#include <hw/pic.hpp>
#include <kernel/irq_manager.hpp>
#include <cstdio>
#include <debug>
#include <info>

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

extern "C" {
  void apic_enable();
  extern void reboot();
  extern char _binary_apic_boot_bin_start;
  extern char _binary_apic_boot_bin_end;
  void lapic_exception_handler() {
    INFO("APIC", "Oops! Exception\n");
    asm volatile("iret");
  }
  unsigned boot_counter = 0;
}

namespace hw {
  
  static const uintptr_t BOOTLOADER_LOCATION = 0x80000;
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
    apic_reg		intr_lo; 	    // ICR1
    apic_reg		intr_hi;      // ICR2
    apic_reg 		timer_vector; // LVTT
    apic_reg reserved33;
    apic_reg reserved34;      // perf count lvt
    apic_reg 		lint0_vector;
    apic_reg 		lint1_vector;
    apic_reg 		error_vector; // err vector
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
  static apic lapic;
  
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
    const uint64_t APIC_BASE_MSR = RDMSR(IA32_APIC_BASE);
    /// find the LAPICs base address ///
    const uintptr_t APIC_BASE_ADDR = APIC_BASE_MSR & 0xFFFFF000;
    printf("APIC base addr: 0x%x\n", APIC_BASE_ADDR);
    // acquire infos
    lapic = apic(APIC_BASE_ADDR);
    printf("LAPIC id: %x  ver: %x\n", lapic.get_id(), lapic.regs->lapic_ver.reg);
    
    /// initialize and start registered APs found in ACPI-tables ///
    if (ACPI::get_cpus().size() > 1) {
      init_smp();
    }
    
    /// enable interrupts ///
    // clear task priority reg to enable interrupts
    lapic.regs->task_pri.reg       = 0;
    lapic.regs->dest_format.reg    = 0xffffffff; // flat mode
    lapic.regs->logical_dest.reg   = 0x01000000; // logical ID 1
    
    // turn the APIC on and enable interrupts
    INFO("APIC", "Enabling interrupts");
    apic_enable();
    // start receiving interrupts (0x100), set spurious vector
    // note: spurious IRQ must have 4 last bits set (0x?F)
    const uint8_t SPURIOUS_IRQ = 0x3f; // IRQ 63
    lapic.enable_intr(SPURIOUS_IRQ);
    
    // disable the legacy 8259 PIC
    hw::outb(0xa1, 0xff);
    hw::outb(0x21, 0xff);
    // mask all interrupts for legacy PIC
    hw::PIC::set_intr_mask(0xFFFF);
    
    // initialize I/O APICs
    IOAPIC::init(ACPI::get_ioapics());
    
    /*
    // wakeup APs
    for (auto& cpu : ACPI::get_cpus())
      // except the CPU we are using now
      if (cpu.id != lapic.get_id())
        send_ipi(cpu.id, 0x80);
    
    reboot();
    */
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
    // populate IDT
    auto* idt_array = (idt_loc*) 0x80480;
    idt_array->limit = 32 * sizeof(IDTDescr) - 1;
    idt_array->base  = (uintptr_t) new IDTDescr[32];
    
    auto* idt = (IDTDescr*) idt_array->base;
    for (size_t i = 0; i < 32; i++) {
      addr_union addr(&lapic_exception_handler);
      idt[i].offset_1 = addr.part[0];
      idt[i].offset_2 = addr.part[1];
      idt[i].selector  = 0x8;
      idt[i].type_attr = 0x8e;
      idt[i].zero      = 0;
    }
    
    // reset counter
    boot_counter = 1;
    size_t CPUcount = ACPI::get_cpus().size();
    
    boot->worker_addr = (void*) &revenant_main;
    boot->stack_base = aligned_alloc(CPUcount * REV_STACK_SIZE, 4096);
    boot->stack_size = REV_STACK_SIZE;
    printf("APIC stack base: %p  size: %u   main size: %u\n", 
        boot->stack_base, boot->stack_size, sizeof(boot->worker_addr));
    
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
      if (cpu.id != lapic.get_id())
        // Send SIPI with start address 0x80000
        lapic.ap_start(cpu.id, 0x80);
    }
    
    // wait for all to start
    while (boot_counter < CPUcount) {
      asm("nop");
    }
    INFO("APIC", "All APs are online now\n");
  }
  
  uint8_t APIC::get_isr()
  {
    for (uint8_t i = 0; i < 8; i++)
      if (lapic.regs->isr[i].reg)
        return 32 * i + __builtin_ffs(lapic.regs->isr[i].reg);
    return 0;
  }
  uint8_t APIC::get_irr()
  {
    for (uint8_t i = 0; i < 8; i++)
      if (lapic.regs->irr[i].reg)
        return 32 * i + __builtin_ffs(lapic.regs->irr[i].reg);
    return 0;
  }
  
  void APIC::eoi(uint8_t)
  {
    printf("-> eoi @ %p\n", &lapic.regs->eoi.reg);
    lapic.regs->eoi.reg = 1;
  }
  
  void APIC::send_ipi(uint8_t id, uint8_t vector)
  {
    // TODO: this doesn't work yet because we need to enable
    // to prevent bad things(tm) from happening, disable interrupts
    asm("cli");
    // IPIs on the IO APIC
    printf("send_ipi  id %u  vector %u\n", id, vector);
    uint32_t value = lapic.regs->intr_hi.reg;
    
    value &= 0x00FFFFFF;
    value |= id << 24;
    lapic.regs->intr_hi.reg = value;
    
    value = ICR_ASSERT | ICR_DLV_STATUS | ICR_FIXED | vector;
    lapic.regs->intr_lo.reg = value;
    asm("sti");
  }
  
  void APIC::reboot()
  {
    ::reboot();
  }
}
