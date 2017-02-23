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

struct per_cpu_test
{
  int value;
  // should not cause deadlock, since only this CPU has access
  spinlock_t testlock = 0;
  
} __attribute__((aligned(64)));
static std::array<per_cpu_test, 16> testing;

spinlock_t writesync;
int        times = 0;

void Service::start()
{
  OS::add_stdout_default_serial();
  printf("This CPU id: %d\n", SMP::cpu_id());

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
    // the job
    __sync_fetch_and_or(&job, 1 << i);
  }, 
  [i] {
    // job completion
    completed++;
      lock(writesync);
      printf("compl = %d\n", completed);
      unlock(writesync);
    
    if (completed == TASKS) {
      lock(writesync);
      printf("All jobs are done now, compl = %d\n", completed);
      printf("bits = %#x\n", job);
      unlock(writesync);
      assert(job = 0xffffffff && "All 32 bits must be set");
    }
  });
  // start working on tasks
  SMP::signal();

  static int event_looped = 0;

  // have one CPU enter an event loop
  for (int i = 1; i < SMP::cpu_count(); i++)
  SMP::enter_event_loop(
  [] {
    lock(writesync);
    printf("AP %d entering event loop\n", SMP::cpu_id());
    event_looped++;
    if (event_looped == SMP::cpu_count()-1) {
        printf("*** All APs have entered event loop\n");
    }
    unlock(writesync);
  });
  // start working on tasks
  SMP::signal();

  void late_fix(int);
  Timers::oneshot(std::chrono::seconds(1), late_fix);
}

void late_fix(int)
{
  printf("interrupting...\n");
  asm ("int $0x7e");
}
