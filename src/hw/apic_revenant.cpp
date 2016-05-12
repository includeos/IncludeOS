#include <hw/apic_revenant.hpp>
#include <cstdio>
//#include <atomic> //< ALFRED!!!!!!!!

static spinlock_t glock = 0;
extern unsigned boot_counter;
// expensive, but correctly returns the current CPU id
extern "C" int get_cpu_id();

void revenant_main(int cpu, uintptr_t esp)
{
  // we can use shared memory here because the
  // bootstrap CPU is waiting on revenants to start
  // we do, however, need to synchronize in between CPUs
  lock(glock);
  printf("\t\tAP %u started at 0x%x\n", cpu, esp);
  unlock(glock);
  // signal that the revenant has started
  asm volatile("lock incl %0" : "=m"(boot_counter));
  
  while (true)
  {
    // sleep
    asm volatile("cli; hlt;");
    
    // do something useful
    lock(glock);
    printf("\t\tAP %u WORKING NOW 0x%x\n", cpu, esp);
    unlock(glock);
  }
}
