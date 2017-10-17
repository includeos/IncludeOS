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

#include <kernel/events.hpp>
#include <cassert>
#include <statman>
#include <smp>
//#define DEBUG_SMP

static std::array<Events, SMP_MAX_CORES> managers;

Events& Events::get(int cpuid)
{
#ifndef INCLUDEOS_SINGLE_THREADED
  return managers.at(cpuid);
#else
  (void) cpuid;
  return managers[0];
#endif
}
Events& Events::get()
{
  return PER_CPU(managers);
}

void Events::init_local()
{
  // prevent legacy IRQs from being free for taking
  for (uint8_t id = 0; id < 32; id++)
      event_subs.set(id);
}

uint8_t Events::subscribe(event_callback func)
{
  auto evt = event_subs.first_free();
  subscribe(evt, func);
  return evt;
}
void Events::subscribe(uint8_t evt, event_callback func)
{
  assert(evt < NUM_EVENTS);

  // enable IRQ in hardware
  __arch_subscribe_irq(evt);

  // Mark as subscribed to
  event_subs.atomic_set(evt);

  // Set callback for event
  callbacks[evt] = func;
#ifdef DEBUG_SMP
  SMP::global_lock();
  printf("Subscribed to intr=%u irq=%u on cpu %d\n",
         IRQ_BASE + evt, evt, SMP::cpu_id());
  SMP::global_unlock();
#endif
}
void Events::unsubscribe(uint8_t evt)
{
  event_subs.atomic_reset(evt);
  callbacks[evt] = nullptr;
}

void Events::process_events()
{
  while (true)
  {
    // event bits that are both pending and subscribed to
    event_todo.set_from_and(event_subs, event_pend);

    int intr = event_todo.first_set();
    if (intr == -1) break;

    do {
      // reset pending before running handler
      event_pend.atomic_reset(intr);
      // sub and call handler
#ifdef DEBUG_SMP
      SMP::global_lock();
      if (intr != 0)
      printf("[cpu%d] Calling handler for intr=%u irq=%u\n",
             SMP::cpu_id(), IRQ_BASE + intr, intr);
      SMP::global_unlock();
#endif
      callbacks[intr]();

      // increment events handled
      handled_array[intr]++;

      event_todo.reset(intr);
      intr = event_todo.first_set();
    }
    while (intr != -1);
  }
}
