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
idt_loc   smp_lapic_idt;

struct segtable
{
  struct GDT gdt;
} __attribute__((aligned(128)));
segtable  gdtables[16];

struct cpu_stuff
{
  int cpduid;
  
} __attribute__((aligned(128)));
cpu_stuff cpudata[16];

void revenant_main(int cpu)
{
  // enable Local APIC
  x86::APIC::get().smp_enable();

  // initialize GDT for this core
  gdtables[cpu].gdt.initialize();
  // create PER-CPU segment
  int fs = gdtables[cpu].gdt.create_data(&cpudata[cpu], 1);
  // load GDT and refresh segments
  GDT::reload_gdt(gdtables[cpu].gdt);
  // enable per-cpu for this core
  cpudata[cpu].cpduid = cpu;
  GDT::set_fs(fs);

  // we can use shared memory here because the
  // bootstrap CPU is waiting on revenants to start
  // we do, however, need to synchronize in between CPUs
  lock(smp.glock);
  INFO("REV", "AP %d started at %#x", get_cpu_id(), get_cpu_esp());
  unlock(smp.glock);
  
  // load IDT
  asm volatile("lidt %0" : : "m"(smp_lapic_idt));
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
    
    // add done function to completed list (only if its callable)
    lock(smp.flock);
    smp.completed.push_back(task.done);
    unlock(smp.flock);
    
    // at least one thread will empty the task list
    if (empty)
      x86::APIC::get().send_bsp_intr();
  }
  __builtin_unreachable();
}

extern void print_backtrace();

extern "C" {
  extern void (*current_eoi_mechanism)();
  void lapic_irq_handler()
  {
    x86::APIC::get().eoi();
    //(*current_eoi_mechanism)();
  }
  void lapic_except_handler()
  {
    lock(smp.glock);
    /// Oops!
    kprintf(">>> LAPIC %d: Oops! CPU Exception!\n", get_cpu_id());
    /// Backtrace
    print_backtrace();
    unlock(smp.glock);
  }
}
