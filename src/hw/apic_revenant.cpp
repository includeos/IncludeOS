#include <hw/apic_revenant.hpp>

#include <hw/apic.hpp>
#include <kernel/irq_manager.hpp>
#include <cstdio>
//#include <atomic> //< ALFRED!!!!!!!!

smp_stuff smp;
idt_loc   smp_lapic_idt;

// expensive, but correctly returns the current CPU id
extern "C" int get_cpu_id();
extern "C" void lapic_irq_handler(int id);
extern "C" void lapic_exception_handler();

void smp_enable_self()
{
  #define LAPIC      0xfee00000
  #define LAPIC_SPURIOUS  0x0f0
  
  auto* spurious_vector = (uint32_t*) (LAPIC | LAPIC_SPURIOUS);
  *spurious_vector = 0x100 | 0x2f;
}

void revenant_main(int cpu, uintptr_t esp)
{
  // load IDT
  asm volatile("lidt %0" : : "m"(smp_lapic_idt));
  // enable Local APIC
  hw::APIC::enable();
  
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
  
  while (true)
  {
    // sleep
    asm volatile("hlt;");
    
    // do something useful
    smp.task_func(get_cpu_id());
    // signal done
    smp.task_barrier.inc();
  }
}

void lapic_exception_handler()
{
  lock(smp.glock);
  printf("LAPIC %d: Oops! Exception %u\n", get_cpu_id(), hw::APIC::get_isr());
  unlock(smp.glock);
  asm volatile("iret");
}

void lapic_irq_handler(int id)
{
  uint8_t vector = hw::APIC::get_isr();
  hw::APIC::eoi();
  
  lock(smp.glock);
  printf("LAPIC %u interrupted on vector %u!\n", id, vector);
  unlock(smp.glock);
}
