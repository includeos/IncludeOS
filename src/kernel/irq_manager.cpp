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
#include "../arch/x86/apic.hpp"
#include <cassert>
#include <statman>
#include <kprint>
#include <smp>

//#define DEBUG_SMP
#define LAPIC_IRQ_BASE   120
#define RING0_CODE_SEG   0x8

static std::array<IRQ_manager, SMP_MAX_CORES> managers;

IRQ_manager& IRQ_manager::get(int cpuid)
{
#ifndef INCLUDEOS_SINGLE_THREADED
  return managers.at(cpuid);
#else
  return managers[0];
#endif
}
IRQ_manager& IRQ_manager::get()
{
  return PER_CPU(managers);
}

uint8_t IRQ_manager::get_free_irq()
{
  return irq_subs.first_free();
}

void IRQ_manager::register_irq(uint8_t irq)
{
  irq_pend.atomic_set(irq);
  count_received[irq]++;
}

extern "C" {
  extern void* get_cpu_esp();
  extern void unused_interrupt_handler();
  extern void modern_interrupt_handler();
  void register_modern_interrupt()
  {
    uint8_t vector = x86::APIC::get_isr();
    IRQ_manager::get().register_irq(vector - IRQ_BASE);
  }
  extern void spurious_intr();
  extern void (*current_eoi_mechanism)();
}

// Only certain exceptions have error codes
template <int NR>
typename std::enable_if<((NR >= 8 and NR <= 15) or NR == 17 or NR == 30)>::type
print_error_code(uint32_t err){
  kprintf("Error code: 0x%x \n", err);
}


// Default: no error code
template <int NR>
typename std::enable_if<((NR < 8 or NR > 15) and NR != 17 and NR != 30)>::type
print_error_code(uint32_t){}


template <int NR>
void cpu_exception(void** eip, uint32_t error)
{
  SMP::global_lock();
  kprintf("\n>>>> !!! CPU %u EXCEPTION !!! <<<<\n", SMP::cpu_id());
  kprintf("Exception %i   EIP  %p\n", NR, eip);
  print_error_code<NR>(error);
  SMP::global_unlock();
  // call panic, which will decide what to do next
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "CPU exception %d", NR);
  panic(buffer);
}


void IRQ_manager::enable_interrupts() {
  asm volatile("sti");
}

void IRQ_manager::init(int cpuid)
{
  get(cpuid).init_local();
}

void IRQ_manager::init_local()
{
  const auto WORDS_PER_BMP = IRQ_LINES / 32;
  auto* bmp = new MemBitmap::word[WORDS_PER_BMP * 3]();
  irq_subs.set_location(bmp + 0 * WORDS_PER_BMP, WORDS_PER_BMP);
  irq_pend.set_location(bmp + 1 * WORDS_PER_BMP, WORDS_PER_BMP);
  irq_todo.set_location(bmp + 2 * WORDS_PER_BMP, WORDS_PER_BMP);

  // prevent get_free_irq from returning taken IDs
  for (uint8_t irq = 0; irq < 32; irq++)
      irq_subs.set(irq);

  if (SMP::cpu_id() == 0)
      INFO("INTR", "Creating exception handlers");
  set_exception_handler(0, cpu_exception<0>);
  set_exception_handler(1, cpu_exception<1>);
  set_exception_handler(2, cpu_exception<2>);
  set_exception_handler(3, cpu_exception<3>);
  set_exception_handler(4, cpu_exception<4>);
  set_exception_handler(5, cpu_exception<5>);
  set_exception_handler(6, cpu_exception<6>);
  set_exception_handler(7, cpu_exception<7>);
  set_exception_handler(8, cpu_exception<8>);
  set_exception_handler(9, cpu_exception<9>);
  set_exception_handler(10, cpu_exception<10>);
  set_exception_handler(11, cpu_exception<11>);
  set_exception_handler(12, cpu_exception<12>);
  set_exception_handler(13, cpu_exception<13>);
  set_exception_handler(14, cpu_exception<14>);
  set_exception_handler(15, cpu_exception<15>);
  set_exception_handler(16, cpu_exception<16>);
  set_exception_handler(17, cpu_exception<17>);
  set_exception_handler(18, cpu_exception<18>);
  set_exception_handler(19, cpu_exception<19>);
  set_exception_handler(20, cpu_exception<20>);
  set_exception_handler(30, cpu_exception<30>);

  if (SMP::cpu_id() == 0)
      INFO2("+ Default interrupt gates set for irq >= 32");
  for (size_t i = 32; i < INTR_LINES - 1; i++){
    set_handler(i, unused_interrupt_handler);
  }
  // spurious interrupt handler
  set_handler(INTR_LINES - 1, spurious_intr);

  // Load IDT
  idt_loc idt_reg;
  idt_reg.limit = INTR_LINES * sizeof(IDTDescr) - 1;
  idt_reg.base = (uint32_t) idt;
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
  create_gate(&idt[vec], func, RING0_CODE_SEG, 0x8e);
}

void IRQ_manager::set_exception_handler(uint8_t vec, exception_func func) {
  create_gate(&idt[vec], (intr_func) func, RING0_CODE_SEG, 0x8e);
}

void IRQ_manager::set_irq_handler(uint8_t irq, intr_func func)
{
  set_handler(irq + IRQ_BASE, func);
}

void IRQ_manager::enable_irq(uint8_t irq) {
  // program IOAPIC to redirect this irq to BSP LAPIC
  __arch_enable_legacy_irq(irq);
}
void IRQ_manager::disable_irq(uint8_t irq) {
  __arch_disable_legacy_irq(irq);
}

void IRQ_manager::subscribe(uint8_t irq, irq_delegate del, bool create_stat)
{
  if (irq >= IRQ_LINES)
    panic("IRQ value out of range (too high)!");

  // cheap way of changing from unused handler to event loop irq marker
  set_irq_handler(irq, modern_interrupt_handler);

  // Mark IRQ as subscribed to
  irq_subs.atomic_set(irq);

  if (create_stat)
  {
    Stat& subscribed = Statman::get().create(Stat::UINT64,
        "cpu" + std::to_string(SMP::cpu_id()) + ".irq" + std::to_string(irq));
    count_handled[irq] = &subscribed.get_uint64();
  }

  // Add callback to subscriber list (for now overwriting any previous)
  irq_delegates_[irq] = del;
#ifdef DEBUG_SMP
  SMP::global_lock();
  printf("Subscribed to intr=%u irq=%u on cpu %d\n",
        IRQ_BASE + irq, irq, SMP::cpu_id());
  SMP::global_unlock();
#endif
}
void IRQ_manager::unsubscribe(uint8_t irq)
{
  irq_subs.atomic_reset(irq);
  irq_delegates_[irq] = nullptr;
}

void IRQ_manager::process_interrupts()
{
  while (true)
  {
    // Get the IRQ's that are both pending and subscribed to
    irq_todo.set_from_and(irq_subs, irq_pend);

    int intr = irq_todo.first_set();
    if (intr == -1) break;

    do {
      // reset pending before running handler
      irq_pend.atomic_reset(intr);
      // sub and call handler
#ifdef DEBUG_SMP
      SMP::global_lock();
      printf("[%p] Calling handler for intr=%u irq=%u cpu %d\n",
             get_cpu_esp(), IRQ_BASE + intr, intr, SMP::cpu_id());
      SMP::global_unlock();
#endif
      irq_delegates_[intr]();

      // increase stat counter, if it exists
      if (count_handled[intr])
          (*count_handled[intr])++;

      irq_todo.reset(intr);
      intr = irq_todo.first_set();
    }
    while (intr != -1);
  }
}
