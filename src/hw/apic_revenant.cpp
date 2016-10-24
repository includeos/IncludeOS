#include <hw/apic_revenant.hpp>

#include <hw/apic.hpp>
#include <kernel/irq_manager.hpp>
#include <cstdio>
#include <unistd.h>

smp_stuff smp;
idt_loc   smp_lapic_idt;

namespace hw {
  extern void _lapic_enable();
}

// expensive, but correctly returns the current CPU id
extern "C" int       get_cpu_id();
extern "C" uintptr_t get_cpu_esp();
extern "C" void      lapic_exception_handler();
#define INFO(FROM, TEXT, ...) printf("%13s ] " TEXT "\n", "[ " FROM, ##__VA_ARGS__)

void revenant_main(int cpu)
{
  // load IDT
  asm volatile("lidt %0" : : "m"(smp_lapic_idt));
  // enable Local APIC
  hw::_lapic_enable();
  
  // we can use shared memory here because the
  // bootstrap CPU is waiting on revenants to start
  // we do, however, need to synchronize in between CPUs
  lock(smp.glock);
  INFO("REV", "AP %d started at %#x", cpu, get_cpu_esp());
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

extern void print_backtrace();
void lapic_exception_handler()
{
  char buffer[1024];
  
  asm volatile("cli");
  lock(smp.glock);
  /// Oops!
  int len = snprintf(buffer, sizeof(buffer), 
            "LAPIC %d: Oops! CPU Exception!\n", get_cpu_id());
  write(1, buffer, len);
  /// Backtrace
  print_backtrace();
  unlock(smp.glock);
  asm volatile("iret");
}
