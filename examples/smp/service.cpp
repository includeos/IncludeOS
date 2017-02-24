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

#include <os>
#include <cassert>
#include <smp>
#include <timers>
#include <kernel/irq_manager.hpp>

struct per_cpu_test
{
  int value;
  
} __attribute__((aligned(64)));
static std::array<per_cpu_test, 16> testing;

static int times = 0;

static void random_irq_handler()
{
  SMP::global_lock();
  times++;
  bool done = (times == SMP::cpu_count()-1);
  SMP::global_unlock();

  if (done) {
    SMP::global_lock();
    printf("Random IRQ handler called %d times\n", times);
    SMP::global_unlock();
    OS::shutdown();
  }
}

void Service::start()
{
  OS::add_stdout_default_serial();

  for (size_t i = 0; i < testing.size(); i++) {
    testing[i].value = i;
  }

  static int completed = 0;
  static uint32_t job = 0;
  static const int TASKS = 8 * sizeof(job);

  // schedule tasks
  for (int i = 0; i < TASKS; i++)
  SMP::add_task(
  [i] {
    // the job is to set the bit if the per-cpu
    // value matches the CPU id.. just as a test
    if (PER_CPU(testing).value == SMP::cpu_id())
        __sync_fetch_and_or(&job, 1 << i);
  }, 
  [i] {
    // job completion
    completed++;
    
    if (completed == TASKS) {
      SMP::global_lock();
      printf("All jobs are done now, compl = %d\n", completed);
      printf("bits = %#x\n", job);
      assert(job = 0xffffffff && "All 32 bits must be set");
      SMP::global_unlock();
    }
  });
  // start working on tasks
  SMP::signal();

  static int event_looped = 0;

  // have one CPU enter an event loop
  for (int i = 1; i < SMP::cpu_count(); i++)
  SMP::enter_event_loop(
  [] {
    SMP::global_lock();
    event_looped++;
    if (event_looped == SMP::cpu_count()-1) {
        printf("*** All APs have entered event loop\n");
    }
    SMP::global_unlock();
    
    const uint8_t IRQ = 110;
    IRQ_manager::get().subscribe(IRQ, random_irq_handler);
    // trigger interrupt
    IRQ_manager::get().register_irq(IRQ);
  });
  // start working on tasks
  SMP::signal();
}
