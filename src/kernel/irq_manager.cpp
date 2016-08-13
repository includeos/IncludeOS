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

//#define DEBUG // Enable debugging
//#define DEBUG2
#include <kernel/irq_manager.hpp>
#include <kernel/syscalls.hpp>
#include <kernel/elf.hpp>
#include <hw/apic.hpp>
#include <cassert>

#define MSIX_IRQ_BASE     64
#define LAPIC_IRQ_BASE   120

uint8_t IRQ_manager::get_next_msix_irq()
{
  static uint8_t next_msix_irq = MSIX_IRQ_BASE;
  return next_msix_irq++;
}

void IRQ_manager::register_irq(uint8_t vector)
{
  irq_pend.atomic_set(vector);
}

extern "C" {
  extern void unused_interrupt_handler();
  extern void modern_interrupt_handler();
  void register_modern_interrupt()
  {
    uint8_t vector = hw::APIC::get_isr();
    IRQ_manager::cpu(0).register_irq(vector - IRQ_BASE);
  }
  void spurious_intr();
  void exception_handler() __attribute__((noreturn));
  extern void (*current_eoi_mechanism)();
}

void exception_handler()
{
  printf("ISR: 0x%x  IRR: 0x%x\n",
    hw::APIC::get_isr(), hw::APIC::get_irr());

  printf(">>>> !!! CPU EXCEPTION !!! <<<<\n");
  panic(">>>> !!! CPU EXCEPTION !!! <<<<\n");
}

void IRQ_manager::enable_interrupts() {
  asm volatile("sti");
}

void IRQ_manager::init()
{
  cpu(0).bsp_init();
}

void IRQ_manager::bsp_init()
{
  const auto WORDS_PER_BMP = IRQ_LINES / 32;
  auto* bmp = new MemBitmap::word[WORDS_PER_BMP * 4]();
  irq_subs.set_location(bmp + 0 * WORDS_PER_BMP, WORDS_PER_BMP);
  irq_pend.set_location(bmp + 1 * WORDS_PER_BMP, WORDS_PER_BMP);
  irq_todo.set_location(bmp + 2 * WORDS_PER_BMP, WORDS_PER_BMP);
  irq_mask.set_location(bmp + 3 * WORDS_PER_BMP, WORDS_PER_BMP);
  // set all bits in the mask
  irq_mask.set_all();

  //Create an idt entry for the 'lidt' instruction
  idt_loc idt_reg;
  idt_reg.limit = INTR_LINES * sizeof(IDTDescr) - 1;
  idt_reg.base = (uint32_t)idt;

  INFO("INTR", "Creating interrupt handlers");

  for (size_t i = 0; i < 32; i++) {
    create_gate(&idt[i],exception_handler,default_sel,default_attr);
  }
  
  // Set all interrupt-gates >= 32 to unused do-nothing handler
  for (size_t i = 32; i < INTR_LINES; i++) {
    create_gate(&idt[i],unused_interrupt_handler,default_sel,default_attr);
  }

  // spurious interrupts (should not happen)
  // currently set to be the last interrupt vector
  create_gate(&idt[INTR_LINES-1], spurious_intr, default_sel, default_attr);

  INFO2("+ Default interrupt gates set for irq >= 32");

  // Load IDT
  asm volatile ("lidt %0": :"m"(idt_reg) );
}

// A union to be able to extract the lower and upper part of an address
union addr_union {
  uint32_t whole;
  struct {
    uint16_t lo16;
    uint16_t hi16;
  };
};

void IRQ_manager::create_gate(
    IDTDescr* idt_entry,
    intr_func func,
    uint16_t segment_sel,
    char attributes)
{
  addr_union addr;
  addr.whole           = (uint32_t) func;
  idt_entry->offset_1  = addr.lo16;
  idt_entry->offset_2  = addr.hi16;
  idt_entry->selector  = segment_sel;
  idt_entry->type_attr = attributes;
  idt_entry->zero      = 0;
}

IRQ_manager::intr_func IRQ_manager::get_handler(uint8_t vec) {
  addr_union addr;
  addr.lo16 = idt[vec].offset_1;
  addr.hi16 = idt[vec].offset_2;

  return (intr_func) addr.whole;
}
IRQ_manager::intr_func IRQ_manager::get_irq_handler(uint8_t irq)
{
  return get_handler(irq + IRQ_BASE);
}

void IRQ_manager::set_handler(uint8_t vec, intr_func func) {
  create_gate(&idt[vec], func, default_sel, default_attr);
}
void IRQ_manager::set_irq_handler(uint8_t irq, intr_func func)
{
  set_handler(irq + IRQ_BASE, func);
}

void IRQ_manager::enable_irq(uint8_t irq) {
  // program IOAPIC to redirect this irq to BSP LAPIC
  hw::APIC::enable_irq(irq);
}

void IRQ_manager::subscribe(uint8_t irq, irq_delegate del) {
  if (irq >= IRQ_LINES)
  {
    printf("IRQ out of bounds %u (max: %u)\n", irq, IRQ_LINES);
    panic("Vector number too high in subscribe()\n");
  }
  
  // cheap way of changing from unused handler to event loop irq marker
  set_irq_handler(irq, modern_interrupt_handler);

  // Mark IRQ as subscribed to
  irq_subs.set(irq);

  // Add callback to subscriber list (for now overwriting any previous)
  irq_delegates_[irq] = del;

  (*current_eoi_mechanism)();
  INFO("IRQ manager", "IRQ subscribed: %u", irq);
}

void IRQ_manager::notify() {

  // Get the IRQ's that are both pending and subscribed to
  irq_todo.set_from_and(irq_subs, irq_pend);
  int intr = irq_todo.last_set();

  while (intr != -1) {

    // reset pending before running handler
    irq_pend.atomic_reset(intr);

    // sub and call handler
    irq_delegates_[intr]();

    // prevent this irq from immediately happening again
    irq_mask.reset(intr);
    
    // rebuild todo-bits
    irq_todo.set_from_and(irq_subs, irq_pend);
    // mask out interrupts we already visited
    irq_todo &= irq_mask;
    
    // find next interrupt
    intr = irq_todo.last_set();
    
    // if no interrupts were found, try again with no mask
    if (intr == -1) {
      // set all mask bits again
      irq_mask.set_all();
      // reset the todo bits
      irq_todo.set_from_and(irq_subs, irq_pend);
      // get new intr value
      intr = irq_todo.last_set();
    }
  }

  debug2("<IRQ notify> Done. OS going to sleep.\n");
  asm volatile("hlt;");
}
