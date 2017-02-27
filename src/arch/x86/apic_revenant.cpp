#include "apic_revenant.hpp"

#include "apic.hpp"
#include "apic_timer.hpp"
#include "gdt.hpp"
#include <kernel/irq_manager.hpp>
#include <kprint>

extern "C" int       get_cpu_id();
extern "C" uintptr_t get_cpu_esp();
extern "C" void      lapic_exception_handler();
#define INFO(FROM, TEXT, ...) printf("%13s ] " TEXT "\n", "[ " FROM, ##__VA_ARGS__)

using namespace x86;
smp_stuff smp;

static bool revenant_task_doer()
{
  // grab hold on task list
  lock(smp.tlock);
  
  if (smp.tasks.empty()) {
    unlock(smp.tlock);
    // try again
    return false;
  }
  
  // get copy of shared task
  auto task = std::move(smp.tasks.front());
  smp.tasks.pop_front();
  
  unlock(smp.tlock);
  
  // execute actual task
  task.func();
  
  // add done function to completed list (only if its callable)
  if (task.done)
  {
    lock(smp.flock);
    smp.completed.push_back(std::move(task.done));
    unlock(smp.flock);
    return true;
  }
  // we did work, but we aren't going to signal back
  return false;
}
static void revenant_task_handler()
{
  bool work_done = false;
  while (true) {
    bool did_something = revenant_task_doer();
    work_done = work_done || did_something;
    if (did_something == false) break;
  }
  if (work_done) {
    x86::APIC::get().send_bsp_intr();
  }
}

void revenant_main(int cpu)
{
  // enable Local APIC
  x86::APIC::get().smp_enable();
  // setup GDT & per-cpu feature
  initialize_gdt_for_cpu(cpu);
  // newlibs printf just does way too much static stuff to work in SMP
  // it can work if REENTs are initalized per-cpu ... and other things
  ::SMP::global_lock();
  INFO2("AP %d started at %#x", get_cpu_id(), get_cpu_esp());
  ::SMP::global_unlock();

  IRQ_manager::init(cpu);
  // enable interrupts
  IRQ_manager::enable_interrupts();
  // init timer system
  APIC_Timer::init();

  IRQ_manager::get().subscribe(0, revenant_task_handler);
  IRQ_manager::get().subscribe(1, APIC_Timer::start_timers);

  // allow programmers to do stuff on each core at init
  ::SMP::init_task();

  // signal that the revenant has started
  smp.boot_barrier.inc();

  while (true)
  {
    IRQ_manager::get().process_interrupts();
    asm volatile("hlt");
  }
  __builtin_unreachable();
}
