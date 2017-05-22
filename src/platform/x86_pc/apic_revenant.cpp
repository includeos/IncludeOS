#include "apic_revenant.hpp"

#include "apic.hpp"
#include "apic_timer.hpp"
#include <kernel/irq_manager.hpp>
#include <kernel/rng.hpp>
#include <kprint>

extern "C" int    get_cpu_id();
extern "C" void*  get_cpu_esp();
extern "C" void   lapic_exception_handler();
#define INFO(FROM, TEXT, ...) printf("%13s ] " TEXT "\n", "[ " FROM, ##__VA_ARGS__)

using namespace x86;
smp_stuff smp_main;
SMP_ARRAY<smp_system_stuff> smp_system;

static bool revenant_task_doer(smp_system_stuff& system)
{
  // grab hold on task list
  lock(system.tlock);

  if (system.tasks.empty()) {
    unlock(system.tlock);
    // try again
    return false;
  }

  // get copy of shared task
  auto task = std::move(system.tasks.front());
  system.tasks.pop_front();

  unlock(system.tlock);

  // execute actual task
  task.func();

  // add done function to completed list (only if its callable)
  if (task.done)
  {
    // NOTE: specifically pushing to 'smp' here, and not 'system'
    lock(smp_main.flock);
    smp_main.completed.push_back(std::move(task.done));
    unlock(smp_main.flock);
    return true;
  }
  // we did work, but we aren't going to signal back
  return false;
}
static void revenant_task_handler()
{
  bool work_done     = false;
  bool did_something = false;
  do {
    // cpu-specific tasks
    did_something = revenant_task_doer(PER_CPU(smp_system));
    work_done = work_done || did_something;
    // global tasks (by taking from index 0)
    did_something = revenant_task_doer(smp_system[0]);
    work_done = work_done || did_something;
    // continue as long as something was done
    // because there could be more
  } while (did_something);
  // if we did any work with done functions, signal back
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
  // show we are online, and verify CPU ID is correct
  ::SMP::global_lock();
  INFO2("AP %d started at %p", cpu, get_cpu_esp());
  ::SMP::global_unlock();
  assert(cpu == ::SMP::cpu_id());

  IRQ_manager::init();
  // enable interrupts
  IRQ_manager::enable_interrupts();
  // init timer system
  APIC_Timer::init();
  // subscribe to task and timer interrupts
  IRQ_manager::get().subscribe(0, revenant_task_handler);
  IRQ_manager::get().subscribe(1, APIC_Timer::start_timers);
  // seed RNG
  RNG::init();

  // allow programmers to do stuff on each core at init
  ::SMP::init_task();

  // signal that the revenant has started
  smp_main.boot_barrier.inc();

  while (true)
  {
    IRQ_manager::get().process_interrupts();
    asm volatile("hlt");
  }
  __builtin_unreachable();
}
