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
#include <kernel/events.hpp>

static int irq_times = 0;

struct alignas(SMP_ALIGN) per_cpu_test
{
  int value;

};
static SMP::Array<per_cpu_test> testing;

#include <malloc.h>
void smp_advanced_test()
{
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
      SMP::global_unlock();
      assert(job = 0xffffffff && "All 32 bits must be set");
      if (SMP::cpu_count() == 1)
          printf("SUCCESS\n");
    }
    volatile void* test = calloc(4, 128u);
    assert(test);
    __sw_barrier();
    test = realloc((void*) test, 128u);
    assert(test);
    __sw_barrier();
    free((void*) test);
  });

  // have one CPU enter an event loop
  for (int i = 1; i < SMP::cpu_count(); i++)
  SMP::add_task(
  [] {
    Timers::oneshot(std::chrono::seconds(1),
    [] (int) {
      static int times = 0;
      SMP::global_lock();
      printf("This is timer from CPU core %d\n", SMP::cpu_id());
      times++;

      if (times     == SMP::cpu_count()-1
       && irq_times == SMP::cpu_count()-1) {
        printf("SUCCESS!\n");
      }
      SMP::global_unlock();
    });
  }, i);
  // start working on tasks
  SMP::signal();
}

static void random_irq_handler()
{
  SMP::global_lock();
  irq_times++;
  bool done = (irq_times == SMP::cpu_count()-1);
  SMP::global_unlock();

  if (done) {
    SMP::global_lock();
    printf("Random IRQ handler called %d times\n", irq_times);
    SMP::global_unlock();
  }
}

static const uint8_t IRQ = 110;
void SMP::init_task()
{
  Events::get().subscribe(IRQ, random_irq_handler);
}

void Service::start()
{

  for (const auto& i : SMP::active_cpus())
  {
    SMP::global_lock();
    printf("CPU %i active \n", i);
    SMP::global_unlock();

    SMP::add_task([i]{
        SMP::global_lock();
        printf("CPU %i, id %i running task \n", i, SMP::cpu_id());
        SMP::global_unlock();
      }, i);

    SMP::signal(i);
  }
  // trigger interrupt
  SMP::broadcast(IRQ);

  // the rest
  smp_advanced_test();
}
