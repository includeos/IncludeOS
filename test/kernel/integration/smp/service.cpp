
#include <os>
#include <cassert>
#include <smp>
#include <timers>
#include <kernel/events.hpp>

static int irq_times = 0;

struct alignas(SMP_ALIGN) per_cpu_test
{
  int value;

};
static std::array<per_cpu_test, 256> testing;

#include <malloc.h>
void smp_advanced_test()
{
  for (size_t i = 0; i < testing.size(); i++) {
    testing[i].value = i;
  }

  static int completed = 0;
  static uint32_t job = 0;
  static const int TASKS = 8 * sizeof(job);

  // schedule tasks
  for (int i = 0; i < TASKS; i++)
  SMP::add_task(
  [i] {
    // the job is to set the bit if the per-cpu
    // value matches the CPU id.. just as a test
    if (PER_CPU(testing).value == SMP::cpu_id())
        __sync_fetch_and_or(&job, 1 << i);
  },
  [] {
    // job completion
    completed++;

    if (completed == TASKS) {
      SMP::global_lock();
      printf("All jobs are done now, compl = %d\n", completed);
      printf("bits = %#x\n", job);
      SMP::global_unlock();
      assert(job = 0xffffffff && "All 32 bits must be set");
    }
	SMP::global_lock();
    volatile void* test = calloc(4, 128u);
    assert(test);
    __sw_barrier();
    test = realloc((void*) test, 128u);
    assert(test);
    __sw_barrier();
    free((void*) test);
	SMP::global_unlock();
  });

  // have one CPU enter an event loop
  for (int i = 1; i < SMP::cpu_count(); i++)
  SMP::add_task(
  [] {
    Timers::oneshot(std::chrono::seconds(1),
    [] (int) {
      static int times = 0;
      SMP::global_lock();
      printf("This is timer from CPU core %d\n", SMP::cpu_id());
      times++;

      if (times     == SMP::cpu_count()-1
       && irq_times == SMP::cpu_count()-1) {
        printf("SUCCESS!\n");
		SMP::add_bsp_task(os::shutdown);
      }
      SMP::global_unlock();
    });
  }, i);
  // start working on tasks
  SMP::signal();
}

#include <smp_utils>
static struct {
	smp_barrier barry;
} messages;

static void random_irq_handler()
{
  SMP::global_lock();
  irq_times++;
  bool done = (irq_times == SMP::cpu_count()-1);

  if (done) {
    printf("Random IRQ handler called %d times\n", irq_times);
  }
  SMP::global_unlock();
}

static const uint8_t IRQ = 110;
void SMP::init_task()
{
  Events::get().subscribe(IRQ, random_irq_handler);
}

#include <thread>
#include <kernel/threads.hpp>
__attribute__((noinline))
static void task_main(int cpu)
{
	SMP::global_lock();
	printf("CPU %d (%d) TID %ld running task\n",
			SMP::cpu_id(), cpu, kernel::get_tid());
	SMP::global_unlock();
}
static void multiprocess_task(int task)
{
	SMP::global_lock();
	printf("CPU %d TASK %d TID %ld running automatic multi-processing task\n",
			SMP::cpu_id(), task, kernel::get_tid());
	SMP::global_unlock();
	messages.barry.increment();
}

void Service::start()
{
  if (SMP::active_cpus().size() < 2) {
	  throw std::runtime_error("Need at least 2 CPUs to run this test!");
  }

  for (const int i : SMP::active_cpus())
  {
	// adding work and signalling CPU=0 is the same as broadcasting to
	// all active CPUs, and giving work to the first free CPU
	if (i == 0) continue; // we don't want to do that here, for this test

    SMP::add_task(
	[cpu = i] {
		auto* t = new std::thread(&task_main, cpu);
		t->join();

		const char TC = kernel::get_tid() & 0xFF;
		volatile void* test = calloc(4, 128u);
	    assert(test);
	    __sw_barrier();
		for (int i = 0; i < 128; i++) {
			((char*)test)[i] = TC;
		}
		__sw_barrier();
	    test = realloc((void*) test, 128u);
	    assert(test);
	    __sw_barrier();
		for (int i = 0; i < 128; i++) {
			assert(((char*)test)[i] == TC);
		}
	    free((void*) test);

		messages.barry.increment();
    }, i);

    SMP::signal(i);
  }

  // wait for idiots to finish
  SMP::global_lock();
  printf("Waiting for %zu tasks from TID=%ld\n",
  		SMP::cpu_count()-1, kernel::get_tid());
  SMP::global_unlock();
  messages.barry.spin_wait(SMP::cpu_count()-1);
  messages.barry.reset();

  // threads will now be migrated to free CPUs
  kernel::setup_automatic_thread_multiprocessing();

  std::vector<std::thread*> mpthreads;
  for (unsigned i = 0; i < SMP::active_cpus().size() - 1; i++)
  {
	mpthreads.push_back(
	  new std::thread(&multiprocess_task, i)
  	);
  }

  SMP::global_lock();
  printf("Joining %zu threads\n", mpthreads.size());
  SMP::global_unlock();

  for (auto* t : mpthreads) {
    t->join();
  }
  // the dead threads should have already made this barrier complete!
  messages.barry.spin_wait(SMP::cpu_count()-1);

  // trigger interrupt
  SMP::broadcast(IRQ);
  // the rest
  smp_advanced_test();
}
