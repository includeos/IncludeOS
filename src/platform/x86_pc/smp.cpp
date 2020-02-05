
#include "smp.hpp"
#include "acpi.hpp"
#include "apic.hpp"
#include "apic_revenant.hpp"
#include "pit.hpp"
#include <os.hpp>
#include <kernel/events.hpp>
#include <kernel/smp_common.hpp>
#include <kernel/threads.hpp>
#include <malloc.h>
#include <algorithm>
#include <cstring>
#include <thread>

extern "C" {
  extern char _binary_apic_boot_bin_start;
  extern char _binary_apic_boot_bin_end;
  extern void __apic_trampoline(); // 64-bit entry
}

static const uintptr_t BOOTLOADER_LOCATION = 0x10000;
static const uint32_t  REV_STACK_SIZE = 1 << 14; // 16kb
static_assert((BOOTLOADER_LOCATION & 0xfff) == 0, "Must be page-aligned");

struct apic_boot {
  // the jump instruction at the start
  uint32_t  padding;
  // stuff we will modify
  uint32_t  worker_addr;
  uint32_t  stack_base;
  uint32_t  stack_size;
};

namespace x86
{

void init_SMP()
{
  const uint32_t CPUcount = ACPI::get_cpus().size();
  // avoid heap usage during AP init
  smp::main_system.initialized_cpus.reserve(CPUcount);
  smp::systems.resize(CPUcount);
  if (CPUcount <= 1) return;

  // copy our bootloader to APIC init location
  const char* start = &_binary_apic_boot_bin_start;
  const ptrdiff_t bootl_size = &_binary_apic_boot_bin_end - start;
  memcpy((char*) BOOTLOADER_LOCATION, start, bootl_size);

  // allocate revenant main stacks
  void* stack = memalign(4096, CPUcount * REV_STACK_SIZE);
  smp::main_system.stack_base = (uintptr_t) stack;
  smp::main_system.stack_size = REV_STACK_SIZE;

  // modify bootloader to support our cause
  auto* boot = (apic_boot*) BOOTLOADER_LOCATION;

#if defined(ARCH_i686)
  boot->worker_addr = (uint32_t) &revenant_main;
#elif defined(ARCH_x86_64)
  boot->worker_addr = (uint32_t) (uintptr_t) &__apic_trampoline;
#else
  #error "Unimplemented arch"
#endif
  boot->stack_base = (uint32_t) smp::main_system.stack_base;
  // add to start at top of each stack, remove to offset cpu 1 to idx 0
  boot->stack_base -= 16;
  boot->stack_size = smp::main_system.stack_size;
  debug("APIC stack base: %#x  size: %u   main size: %u\n",
      boot->stack_base, boot->stack_size, sizeof(boot->worker_addr));
  assert((boot->stack_base & 15) == 0);

  // reset barrier
  smp::main_system.boot_barrier.reset(1);

  auto& apic = x86::APIC::get();
  // massage musl to create a main thread for each AP
  for (const auto& cpu : ACPI::get_cpus())
  {
	  if (cpu.id == apic.get_id() || cpu.id >= CPUcount) continue;
	  // this thread will immediately yield back here
	  new std::thread(&revenant_thread_main, cpu.id);
	  // the last thread id will be the above threads kernel id
	  // alternatively, we can extract this threads last-created childs id
	  const long tid = kernel::get_last_thread_id();
	  // store thread info in SMP structure
	  auto& system = smp::systems.at(cpu.id);
	  system.main_thread_id = tid;
	  // migrate thread to its CPU
	  auto* kthread = kernel::ThreadManager::get().detach(tid);
	  kernel::ThreadManager::get(cpu.id).attach(kthread);
  }

  // turn on CPUs
  INFO("SMP", "Initializing APs");
  for (const auto& cpu : ACPI::get_cpus())
  {
    if (cpu.id == apic.get_id() || cpu.id >= CPUcount) continue;
    debug("-> CPU %u ID %u  fl 0x%x\n",
          cpu.cpu, cpu.id, cpu.flags);
    apic.ap_init(cpu.id);
  }
  //PIT::blocking_cycles(10);

  // start CPUs
  INFO("SMP", "Starting APs");
  for (const auto& cpu : ACPI::get_cpus())
  {
    if (cpu.id == apic.get_id() || cpu.id >= CPUcount) continue;
    // Send SIPI with start page at BOOTLOADER_LOCATION
    apic.ap_start(cpu.id, BOOTLOADER_LOCATION >> 12);
    apic.ap_start(cpu.id, BOOTLOADER_LOCATION >> 12);
  }
  //PIT::blocking_cycles(1);

  // wait for all APs to start
  smp::main_system.boot_barrier.spin_wait(CPUcount);
  INFO("SMP", "All %u APs are online now\n", CPUcount);

  // subscribe to IPIs
  Events::get().subscribe(BSP_LAPIC_IPI_IRQ, smp::task_done_handler);
}

} // x86

using namespace x86;

/// implementation of the SMP interface ///
int SMP::cpu_count() noexcept {
  return smp::main_system.initialized_cpus.size();
}
const std::vector<int>& SMP::active_cpus() {
  return smp::main_system.initialized_cpus;
}
size_t SMP::early_cpu_total() noexcept {
	return ACPI::get_cpus().size();
}

__attribute__((weak))
void SMP::init_task()
{
  /* do nothing */
}

void SMP::add_task(SMP::task_func task, SMP::done_func done, int cpu)
{
  auto& system = PER_CPU(smp::systems);
  system.tlock.lock();
  system.tasks.emplace_back(std::move(task), std::move(done));
  system.tlock.unlock();
}
void SMP::add_task(SMP::task_func task, int cpu)
{
  auto& system = PER_CPU(smp::systems);
  system.tlock.lock();
  system.tasks.emplace_back(std::move(task), nullptr);
  system.tlock.unlock();
}
void SMP::add_bsp_task(SMP::done_func task)
{
  // queue job
  auto& system = PER_CPU(smp::systems);
  system.flock.lock();
  system.completed.push_back(std::move(task));
  system.flock.unlock();
  // set this CPU bit
  smp::main_system.bitmap.atomic_set(SMP::cpu_id());
  // call home
  x86::APIC::get().send_bsp_intr();
}

void SMP::signal(int cpu)
{
  // broadcast that there is work to do
  // 0: Broadcast to everyone except BSP
  if (cpu == 0)
      x86::APIC::get().bcast_ipi(0x20);
  // 1-xx: Unicast specific vCPU
  else
      x86::APIC::get().send_ipi(cpu, 0x20);
}
void SMP::signal_bsp()
{
  x86::APIC::get().send_bsp_intr();
}

void SMP::broadcast(uint8_t irq)
{
  x86::APIC::get().bcast_ipi(IRQ_BASE + irq);
}
void SMP::unicast(int cpu, uint8_t irq)
{
  x86::APIC::get().send_ipi(cpu, IRQ_BASE + irq);
}

static smp_spinlock g_global_lock;

void SMP::global_lock() noexcept
{
  g_global_lock.lock();
}
void SMP::global_unlock() noexcept
{
  g_global_lock.unlock();
}
