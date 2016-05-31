#include <hw/apic_revenant.hpp>

#include <hw/apic.hpp>
#include <kernel/irq_manager.hpp>
#include <cstdio>
//#include <atomic> //< ALFRED!!!!!!!!

smp_stuff smp;
idt_loc   smp_lapic_idt;

namespace hw {
  extern void _lapic_enable();
}

// expensive, but correctly returns the current CPU id
extern "C" int get_cpu_id();
extern "C" void lapic_exception_handler();

void revenant_main(int cpu, uintptr_t esp)
{
  // load IDT
  asm volatile("lidt %0" : : "m"(smp_lapic_idt));
  // enable Local APIC
  hw::_lapic_enable();
  
  // we can use shared memory here because the
  // bootstrap CPU is waiting on revenants to start
  // we do, however, need to synchronize in between CPUs
  lock(smp.glock);
  printf("\t\tAP %u started at 0x%x\n", cpu, esp);
  unlock(smp.glock);
  
  // enable interrupts
  asm volatile("sti");
  
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
    
    bool empty = smp.tasks.empty();
    
    unlock(smp.tlock);
    
    // execute actual task
    task.func();
    
    // add the done function to completed list
    lock(smp.flock);
    smp.completed.push_back(task.done);
    unlock(smp.flock);
    
    // at least one thread will empty the task list
    if (empty)
      hw::APIC::send_bsp_intr();
  }
}

void lapic_exception_handler()
{
  lock(smp.glock);
  printf("LAPIC %d: Oops! Exception %u\n", get_cpu_id(), hw::APIC::get_isr());
  unlock(smp.glock);
  asm volatile("iret");
}
