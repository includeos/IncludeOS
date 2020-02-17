#include "apic_revenant.hpp"
#include "apic.hpp"
#include "apic_timer.hpp"
#include "clocks.hpp"
#include "idt.hpp"
#include "init_libc.hpp"
#include <kernel/events.hpp>
#include <kernel/rng.hpp>
#include <kernel/threads.hpp>
#include <kernel/smp_common.hpp>
#include <os.hpp>

namespace x86 {
  extern void initialize_cpu_tables_for_cpu(int);
}

void revenant_thread_main(int cpu)
{
	while (smp::main_system.still_starting) sched_yield();
	uintptr_t this_stack = smp::main_system.stack_base + cpu * smp::main_system.stack_size;

	static smp_spinlock startup_lock;
    // show we are online, and verify CPU ID is correct
    startup_lock.lock();
    INFO2("AP %d started at %p", SMP::cpu_id(), (void*) this_stack);
    startup_lock.unlock();
    Expects(cpu == SMP::cpu_id());

	auto& ev = Events::get(cpu);
	ev.init_local();
startup_lock.lock();
	// subscribe to task and timer interrupts
	ev.subscribe(0, smp::smp_task_handler);
	ev.subscribe(1, x86::APIC_Timer::start_timers);
	// init timer system
	x86::APIC_Timer::init();
startup_lock.unlock();
	// initialize clocks
	x86::Clocks::init();

#ifndef INCLUDEOS_RNG_IS_SHARED
	// NOTE: its faster if we can steal RNG from main CPU, but not scaleable
	// perhaps its just better to do it like this, or even share RNG
	RNG::get().init();
#endif
	// enable interrupts
	asm volatile("sti");

	// allow programmers to do stuff on each core at init
    SMP::init_task();

    // signal that the revenant has started
    smp::main_system.boot_barrier.increment();

    while (true)
    {
      ev.process_events();
      os::halt();
    }
    __builtin_unreachable();
}

extern "C"
void revenant_main(int cpu)
{
  // enable Local APIC
  x86::APIC::get().smp_enable();
  // setup GDT & per-cpu feature
  x86::initialize_cpu_tables_for_cpu(cpu);
  // initialize exceptions before asserts
  x86::idt_initialize_for_cpu(cpu);

#ifdef ARCH_x86_64
  // interrupt stack tables
  uintptr_t this_stack =
    smp::main_system.stack_base + cpu * smp::main_system.stack_size;
  x86::ist_initialize_for_cpu(cpu, this_stack);

  const uint64_t star_kernel_cs = 8ull << 32;
  const uint64_t star_user_cs   = 8ull << 48;
  x86::CPU::write_msr(IA32_STAR, star_kernel_cs | star_user_cs);
  x86::CPU::write_msr(IA32_LSTAR, (uintptr_t)&__syscall_entry);
#endif

  asm("" : : : "memory");

  auto& system = smp::systems.at(cpu);
  // setup main thread
  auto* kthread = kernel::setup_main_thread(cpu, system.main_thread_id);
  // resume APs main thread
  kthread->resume();
  __builtin_unreachable();
}
