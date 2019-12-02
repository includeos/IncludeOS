
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
static SMP::Array<per_cpu_test> testing;

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
  [i] {
    // job completion
    completed++;

    if (completed == TASKS) {
      SMP::global_lock();
      printf("All jobs are done now, compl = %d\n", completed);
      printf("bits = %#x\n", job);
      SMP::global_unlock();
      assert(job = 0xffffffff && "All 32 bits must be set");
      if (SMP::cpu_count() == 1)
          printf("SUCCESS\n");
    }
    volatile void* test = calloc(4, 128u);
    assert(test);
    __sw_barrier();
    test = realloc((void*) test, 128u);
    assert(test);
    __sw_barrier();
    free((void*) test);
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
      }
      SMP::global_unlock();
    });
  }, i);
  // start working on tasks
  SMP::signal();
}

#include <smp_utils>
static struct {
	std::vector<std::string> buffers;
	spinlock_t spinner;
	minimal_barrier_t barry;
} messages;

__attribute__((format (printf, 1, 2)))
static int smpprintf(const char* fmt, ...) {
	char buffer[4096];
	// printf format -> buffer
	va_list va;
	va_start(va, fmt);
	int len = vsnprintf(buffer, sizeof(buffer), fmt, va);
	va_end(va);
	// serialized append buffer
	scoped_spinlock { messages.spinner };
	messages.buffers.emplace_back(buffer, buffer + len);
	// return message length
	return len;
}


static void random_irq_handler()
{
  SMP::global_lock();
  irq_times++;
  bool done = (irq_times == SMP::cpu_count()-1);
  SMP::global_unlock();

  if (done) {
    smpprintf("Random IRQ handler called %d times\n", irq_times);
  }
}

static const uint8_t IRQ = 110;
void SMP::init_task()
{
  Events::get().subscribe(IRQ, random_irq_handler);
}

#include <thread>
extern "C" void kprintf(const char* format, ...);
#include <kernel/threads.hpp>
extern "C" ssize_t write(int, const void*, size_t);
extern "C" void __serial_print1(const char* cstr);
__attribute__((noinline))
static void task_main(int cpu)
{
	kprintf("CPU %d running task\n", SMP::cpu_id());
}

void Service::start()
{
#ifdef INCLUDEOS_SMP_ENABLE
  printf("SMP is enabled\n");
#else
  static_assert(false, "SMP is not enabled");
#endif

  for (const int i : SMP::active_cpus())
  {
	// adding work and signalling CPU=0 is the same as broadcasting to
	// all active CPUs, and giving work to the first free CPU
	if (i == 0) continue; // we don't want to do that here, for this test
    printf("CPU %d active\n", i);

    SMP::add_task(
	[cpu = i] {
		__serial_print1("__serial_print1 works\n");
		const char buffer[] = "os::print works\n";
		os::default_stdout(buffer, sizeof(buffer)-1);
		os::print(buffer, sizeof(buffer)-1);
		auto* t = new std::thread(&task_main, cpu);
		t->join();
		smpprintf("CPU %d running task (thread=%p)\n", SMP::cpu_id(), kernel::get_thread());
		messages.barry.increment();
    }, i);

    SMP::signal(i);
  }

  // wait for idiots to finish
  messages.barry.spin_wait(SMP::cpu_count()-1);
  for (const auto& msg : messages.buffers) {
	  printf("%.*s", (int) msg.size(), msg.data());
  }
  messages.buffers.clear();

  // trigger interrupt
  SMP::broadcast(IRQ);

  printf("SUCCESS\n");

  // the rest
  //smp_advanced_test();
}
