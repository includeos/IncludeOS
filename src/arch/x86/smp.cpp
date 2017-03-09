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

static const uintptr_t BOOTLOADER_LOCATION = 0x10000;
static const size_t    REV_STACK_SIZE = 1 << 18; // 256kb

struct apic_boot {
  // the jump instruction at the start
  uint32_t  jump;
  // stuff we will modify
  void*     worker_addr;
  char*     stack_base;
  size_t    stack_size;
};

namespace x86
{

void SMP::init()
{
  size_t CPUcount = ACPI::get_cpus().size();
  if (CPUcount <= 1) return;
  assert(CPUcount <= SMP_MAX_CORES);

  // copy our bootloader to APIC init location
  const char* start = &_binary_apic_boot_bin_start;
  ptrdiff_t bootloader_size = &_binary_apic_boot_bin_end - start;
  memcpy((char*) BOOTLOADER_LOCATION, start, bootloader_size);

  // modify bootloader to support our cause
  auto* boot = (apic_boot*) BOOTLOADER_LOCATION;

  boot->worker_addr = (void*) &revenant_main;
  boot->stack_base = (char*) memalign(4096, CPUcount * REV_STACK_SIZE);
  boot->stack_base += REV_STACK_SIZE; // make sure the stack starts at the top
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
    // Send SIPI with start address BOOTLOADER_LOCATION
    apic.ap_start(cpu.id, BOOTLOADER_LOCATION >> 12);
    apic.ap_start(cpu.id, BOOTLOADER_LOCATION >> 12);
  }

  // wait for all APs to start
  smp.boot_barrier.spin_wait(CPUcount);
  INFO("SMP", "All %u APs are online now\n", CPUcount);

  // subscribe to IPIs
  IRQ_manager::get().subscribe(BSP_LAPIC_IPI_IRQ,
  [] {
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

__attribute__((weak))
void ::SMP::init_task()
{
  /* do nothing */
}

void ::SMP::add_task(smp_task_func task, smp_done_func done)
{
#ifdef INCLUDEOS_SINGLE_THREADED
  task(); done();
#else
  lock(smp.tlock);
  smp.tasks.emplace_back(std::move(task), std::move(done));
  unlock(smp.tlock);
#endif
}
void ::SMP::add_task(smp_task_func task)
{
#ifdef INCLUDEOS_SINGLE_THREADED
  task();
#else
  lock(smp.tlock);
  smp.tasks.emplace_back(std::move(task), nullptr);
  unlock(smp.tlock);
#endif
}
void ::SMP::signal()
{
  // broadcast that there is work to do
  x86::APIC::get().bcast_ipi(0x20);
}

void ::SMP::broadcast(uint8_t irq)
{
  x86::APIC::get().bcast_ipi(IRQ_BASE + irq);
}
void ::SMP::unicast(int cpu, uint8_t irq)
{
  x86::APIC::get().send_ipi(cpu, IRQ_BASE + irq);
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

/// SMP variants of malloc and free ///
#ifndef INCLUDEOS_SINGLE_THREADED
static spinlock_t __memory_lock = 0;

#include <malloc.h>
void* malloc(size_t size)
{
  if (SMP::cpu_count() == 1) {
    return _malloc_r(_REENT, size);
  }
  lock(__memory_lock);
  void* addr = _malloc_r(_REENT, size);
  unlock(__memory_lock);
  return addr;
}
void* calloc(size_t num, size_t size)
{
  lock(__memory_lock);
  void* addr = _calloc_r(_REENT, num, size);
  unlock(__memory_lock);
  return addr;
}
void* realloc(void *ptr, size_t new_size)
{
  lock(__memory_lock);
  void* addr = _realloc_r (_REENT, ptr, new_size);
  unlock(__memory_lock);
  return addr;
}
void free(void* ptr)
{
  if (SMP::cpu_count() == 1) {
    return _free_r(_REENT, ptr);
  }
  lock(__memory_lock);
  _free_r(_REENT, ptr);
  unlock(__memory_lock);
}
#endif
