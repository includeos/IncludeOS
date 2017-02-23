#include "apic_revenant.hpp"

#include "gdt.hpp"
#include <kernel/irq_manager.hpp>
#include <kprint>

extern "C" int       get_cpu_id();
extern "C" uintptr_t get_cpu_esp();
extern "C" void      lapic_exception_handler();
#define INFO(FROM, TEXT, ...) printf("%13s ] " TEXT "\n", "[ " FROM, ##__VA_ARGS__)

using namespace x86;
smp_stuff smp;

void revenant_main(int cpu)
{
  // enable Local APIC
  x86::APIC::get().smp_enable();

  initialize_gdt_for_cpu(cpu);

  // we can use shared memory here because the
  // bootstrap CPU is waiting on revenants to start
  // we do, however, need to synchronize in between CPUs
  lock(smp.glock);
  INFO2("AP %d started at %#x", get_cpu_id(), get_cpu_esp());
  unlock(smp.glock);

  IRQ_manager::init(cpu);
  // enable interrupts
  IRQ_manager::enable_interrupts();

  // signal that the revenant has started
  smp.boot_barrier.inc();
  // sleep
  asm volatile("hlt");
  
  while (true)
  {
    // grab hold on task list
    lock(smp.tlock);
    
    if (smp.tasks.empty()) {
      unlock(smp.tlock);
      // sleep
      asm volatile("hlt");
      // try again
      continue;
    }
    
    // get copy of shared task
    auto task = smp.tasks.front();
    smp.tasks.pop_front();
    
    unlock(smp.tlock);
    
    // execute actual task
    task.func();
    
    // add done function to completed list (only if its callable)
    if (true) //task.done)
    {
      lock(smp.flock);
      smp.completed.push_back(task.done);
      unlock(smp.flock);
    }
    
    // at least one thread will empty the task list
    x86::APIC::get().send_bsp_intr();
  }
  __builtin_unreachable();
}
