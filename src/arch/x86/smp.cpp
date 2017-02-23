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

#include "smp.hpp"
#include "acpi.hpp"
#include "apic.hpp"
#include "apic_revenant.hpp"
#include <kernel/os.hpp>
#include <kernel/irq_manager.hpp>
#include <malloc.h>
#include <algorithm>
#include <cstring>

extern "C" {
  extern char _binary_apic_boot_bin_start;
  extern char _binary_apic_boot_bin_end;
}

static const uintptr_t BOOTLOADER_LOCATION = 0x80000;

struct apic_boot {
  // the jump instruction at the start
  uint32_t   jump;
  // stuff we will need to modify
  void*  worker_addr;
  void*  stack_base;
  size_t stack_size;
};

namespace x86
{

void SMP::init()
{
  size_t CPUcount = ACPI::get_cpus().size();
  if (CPUcount <= 1) return;

  // copy our bootloader to APIC init location
  const char* start = &_binary_apic_boot_bin_start;
  ptrdiff_t bootloader_size = &_binary_apic_boot_bin_end - start;
  debug("Copying bootloader from %p to 0x%x (size=%d)\n",
        start, BOOTLOADER_LOCATION, bootloader_size);
  memcpy((char*) BOOTLOADER_LOCATION, start, bootloader_size);

  // modify bootloader to support our cause
  auto* boot = (apic_boot*) BOOTLOADER_LOCATION;

  // assign stack and main func
  boot->worker_addr = (void*) &revenant_main;
  boot->stack_base = memalign(CPUcount * REV_STACK_SIZE, 4096);
  boot->stack_size = REV_STACK_SIZE;
  debug("APIC stack base: %p  size: %u   main size: %u\n",
      boot->stack_base, boot->stack_size, sizeof(boot->worker_addr));

  // reset barrier
  smp.boot_barrier.reset(1);

  auto& apic = x86::APIC::get();
  // turn on CPUs
  INFO("SMP", "Initializing APs");
  for (auto& cpu : ACPI::get_cpus())
  {
    if (cpu.id == apic.get_id()) continue;
    debug("-> CPU %u ID %u  fl 0x%x\n",
          cpu.cpu, cpu.id, cpu.flags);
    apic.ap_init(cpu.id);
  }
  // start CPUs
  INFO("SMP", "Starting APs");
  for (auto& cpu : ACPI::get_cpus())
  {
    if (cpu.id == apic.get_id()) continue;
    // Send SIPI with start address 0x80000
    apic.ap_start(cpu.id, 0x80);
    apic.ap_start(cpu.id, 0x80);
  }

  // wait for all APs to start
  smp.boot_barrier.spin_wait(CPUcount);
  INFO("SMP", "All %u APs are online now\n", CPUcount);

  // subscribe to IPIs
  IRQ_manager::get().subscribe(BSP_LAPIC_IPI_IRQ,
  [] {
    printf("checking...\n");
    // copy all the done functions out from queue to our local vector
    auto done = SMP::get_completed();
    // call all the done functions
    for (auto& func : done) {
      func();
    }
  });
}

std::vector<smp_done_func> SMP::get_completed()
{
  std::vector<smp_done_func> done;
  lock(smp.flock);
  for (auto& func : smp.completed) done.push_back(func);
  smp.completed.clear(); // MUI IMPORTANTE
  unlock(smp.flock);
  return done;
}

} // x86

/// implementation of the SMP interface ///
int ::SMP::cpu_id() noexcept
{
  int cpuid;
  asm volatile("movl %%fs:(0x0), %0" : "=r" (cpuid));
  return cpuid;
}
int ::SMP::cpu_count() noexcept
{
  return x86::ACPI::get_cpus().size();
}

void ::SMP::add_task(smp_task_func task, smp_done_func done)
{
  lock(smp.tlock);
  smp.tasks.emplace_back(task, done);
  unlock(smp.tlock);
}
void ::SMP::enter_event_loop(smp_task_func task)
{
  lock(smp.tlock);
  smp.tasks.emplace_back(
  smp_task_func::make_packed(
  [task] {
    task();
    while (OS::is_running()) {
      IRQ_manager::get().process_interrupts();
      OS::halt();
    }
    }), nullptr);
  unlock(smp.tlock);
}
void ::SMP::signal()
{
  // broadcast that we have work to do
  x86::APIC::get().bcast_ipi(0x20);
}

static spinlock_t __global_lock = 0;
void ::SMP::global_lock() noexcept
{
  lock(__global_lock);
}
void ::SMP::global_unlock() noexcept
{
  unlock(__global_lock);
}
