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
#include <cassert>
#include <statman>
#include <kprint>
#include <smp>
//#define DEBUG_SMP

static std::array<IRQ_manager, SMP_MAX_CORES> managers;

IRQ_manager& IRQ_manager::get(int cpuid)
{
#ifndef INCLUDEOS_SINGLE_THREADED
  return managers.at(cpuid);
#else
  (void) cpuid;
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

void IRQ_manager::init()
{
  get().init_local();
}

void IRQ_manager::init_local()
{
  const auto WORDS_PER_BMP = INTR_LINES / 32;
  auto* bmp = new MemBitmap::word[WORDS_PER_BMP * 3]();
  irq_subs.set_location(bmp + 0 * WORDS_PER_BMP, WORDS_PER_BMP);
  irq_pend.set_location(bmp + 1 * WORDS_PER_BMP, WORDS_PER_BMP);
  irq_todo.set_location(bmp + 2 * WORDS_PER_BMP, WORDS_PER_BMP);

  // prevent get_free_irq from returning taken IDs
  for (uint8_t irq = 0; irq < 32; irq++)
      irq_subs.set(irq);
}

void IRQ_manager::enable_irq(uint8_t irq) {
  // program IOAPIC to redirect this irq to BSP LAPIC
  __arch_enable_legacy_irq(irq);
}
void IRQ_manager::disable_irq(uint8_t irq) {
  __arch_disable_legacy_irq(irq);
}

uint8_t IRQ_manager::subscribe(irq_delegate del)
{
  uint8_t irq = get_free_irq();
  subscribe(irq, del);
  return irq;
}
void IRQ_manager::subscribe(uint8_t irq, irq_delegate del)
{
  if (irq >= IRQ_LINES)
    panic("IRQ value out of range (too high)!");

  // cheap way of changing from unused handler to event loop irq marker
  __arch_subscribe_irq(irq);

  // Mark IRQ as subscribed to
  irq_subs.atomic_set(irq);

  // Create stat for this event
  Stat& subscribed = Statman::get().create(Stat::UINT64,
      "cpu" + std::to_string(SMP::cpu_id()) + ".irq" + std::to_string(irq));
  count_handled[irq] = &subscribed.get_uint64();

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
  if (count_handled[irq]) {
    Statman::get().free(count_handled[irq]);
  }
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
      if (intr != 0)
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
