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
    lock(PER_CPU(smp_system).flock);
    PER_CPU(smp_system).completed.push_back(std::move(task.done));
    unlock(PER_CPU(smp_system).flock);
    // signal home
    PER_CPU(smp_system).work_done = true;
  }
  return true;
}
static void revenant_task_handler()
{
  auto& system = PER_CPU(smp_system);
  system.work_done = false;
  // cpu-specific tasks
  while(revenant_task_doer(PER_CPU(smp_system)));
  // global tasks (by taking from index 0)
  while (revenant_task_doer(smp_system[0]));
  // if we did any work with done functions, signal back
  if (system.work_done) {
    // set bit for this CPU
    smp_main.bitmap.atomic_set(::SMP::cpu_id());
    // signal main CPU
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
