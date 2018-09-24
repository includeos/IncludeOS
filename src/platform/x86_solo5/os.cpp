#include <os>

#include <info>
#include <smp>
#include <statman>
#include <kernel/events.hpp>
#include <kernel/timers.hpp>
#include <kernel/solo5_manager.hpp>

extern "C" {
#include <solo5.h>
}

// sleep statistics
static uint64_t os_cycles_hlt = 0;

extern "C" void* get_cpu_esp();
extern uintptr_t _start;
extern uintptr_t _end;
extern uintptr_t mem_size;
extern char _ELF_START_;
extern char _TEXT_START_;
extern char _LOAD_START_;
extern char _ELF_END_;
// in kernel/os.cpp
extern bool os_default_stdout;

#define MYINFO(X,...) INFO("Kernel", X, ##__VA_ARGS__)

#ifdef ENABLE_PROFILERS
#include <profile>
#define PROFILE(name)  ScopedProfiler __CONCAT(sp, __COUNTER__){name};
#else
#define PROFILE(name) /* name */
#endif

void solo5_poweroff()
{
  __asm__ __volatile__("cli; hlt");
  for(;;);
}

// returns wall clock time in nanoseconds since the UNIX epoch
uint64_t __arch_system_time() noexcept
{
  return solo5_clock_wall();
}
timespec __arch_wall_clock() noexcept
{
  uint64_t stamp = solo5_clock_wall();
  timespec result;
  result.tv_sec = stamp / 1000000000ul;
  result.tv_nsec = stamp % 1000000000ul;
  return result;
}

// actually uses nanoseconds (but its just a number)
uint64_t OS::cycles_asleep() noexcept {
  return os_cycles_hlt;
}
uint64_t OS::nanos_asleep() noexcept {
  return os_cycles_hlt;
}

void OS::default_stdout(const char* str, const size_t len)
{
  solo5_console_write(str, len);
}

void OS::start(const char* cmdline)
{
  OS::cmdline = cmdline;

  // Initialize stdout handlers
  if(os_default_stdout) {
    OS::add_stdout(&OS::default_stdout);
  }

  PROFILE("Global stdout constructors");
  extern OS::ctor_t __stdout_ctors_start;
  extern OS::ctor_t __stdout_ctors_end;
  OS::run_ctors(&__stdout_ctors_start, &__stdout_ctors_end);

  // Call global ctors
  PROFILE("Global kernel constructors");
  extern OS::ctor_t __init_array_start;
  extern OS::ctor_t __init_array_end;
  OS::run_ctors(&__init_array_start, &__init_array_end);

  PROFILE("");
  // Print a fancy header
  CAPTION("#include<os> // Literally");

  void* esp = get_cpu_esp();
  MYINFO("Stack: %p", esp);

  /// STATMAN ///
  /// initialize on page 7, 2 pages in size
  Statman::get().init(0x6000, 0x3000);

  PROFILE("Memory map");
  // Assign memory ranges used by the kernel
  auto& memmap = memory_map();
  MYINFO("Assigning fixed memory ranges (Memory map)");

  memmap.assign_range({0x500, 0x5fff, "solo5"});
  memmap.assign_range({0x6000, 0x8fff, "Statman"});
  memmap.assign_range({0xA000, 0x9fbff, "Stack"});
  memmap.assign_range({(uintptr_t)&_LOAD_START_, (uintptr_t)&_end,
        "ELF"});

  Expects(heap_begin() and heap_max_);
  // @note for security we don't want to expose this
  memmap.assign_range({(uintptr_t)&_end + 1, heap_begin() - 1,
        "Pre-heap"});

  uintptr_t span_max = std::numeric_limits<std::ptrdiff_t>::max();
  uintptr_t heap_range_max_ = std::min(span_max, heap_max_);

  MYINFO("Assigning heap");
  memmap.assign_range({heap_begin(), heap_range_max_,
        "Dynamic memory", heap_usage });

  MYINFO("Printing memory map");
  for (const auto &i : memmap)
    INFO2("* %s",i.second.to_string().c_str());

  extern void __platform_init();
  __platform_init();

  MYINFO("Booted at monotonic_ns=%ld walltime_ns=%ld",
         solo5_clock_monotonic(), solo5_clock_wall());

  extern OS::ctor_t __driver_ctors_start;
  extern OS::ctor_t __driver_ctors_end;
  OS::run_ctors(&__driver_ctors_start, &__driver_ctors_end);

  Solo5_manager::init();

  // We don't need a start or stop function in solo5.
  Timers::init(
    // timer start function
    [] (auto) {},
    // timer stop function
    [] () {});

  Events::get().defer(Timers::ready);
}

static inline void event_loop_inner()
{
  int res = 0;
  auto nxt = Timers::next();
  if (nxt == std::chrono::nanoseconds(0))
  {
    // no next timer, wait forever
    //printf("Waiting 15s, next is indeterminate...\n");
    const unsigned long long count = 15000000000ULL;
    res = solo5_yield(solo5_clock_monotonic() + count);
    os_cycles_hlt += count;
  }
  else if (nxt == std::chrono::nanoseconds(1))
  {
    // there is an imminent or activated timer, don't wait
    //printf("Not waiting, imminent timer...\n");
  }
  else
  {
    //printf("Waiting %llu nanos\n", nxt.count());
    res = solo5_yield(solo5_clock_monotonic() + nxt.count());
    os_cycles_hlt += nxt.count();
  }

  // handle any activated timers
  Timers::timers_handler();
  Events::get().process_events();
  if (res != 0)
  {
    // handle any network traffic
    for(auto& nic : hw::Devices::devices<hw::Nic>()) {
      nic->poll();
    }
  }
}

void OS::event_loop()
{
  while (power_)
  {
    // add a global symbol here so we can quickly discard
    // event loop from stack sampling
    asm volatile(
    ".global _irq_cb_return_location;\n"
    "_irq_cb_return_location:" );

    event_loop_inner();
  }


  MYINFO("Stopping service");
  Service::stop();

  MYINFO("Powering off");
  solo5_poweroff();
}

__attribute__((noinline))
void OS::halt() {
  auto cycles_before = solo5_clock_monotonic();
#if defined(ARCH_x86)
  asm volatile("hlt");
#else
#warning "OS::halt() not implemented for selected arch"
#endif
  // Count sleep nanos
  os_cycles_hlt += solo5_clock_monotonic() - cycles_before;
}

void OS::block()
{
  static uint32_t blocking_level = 0;
  blocking_level += 1;
  // prevent recursion stack overflow
  assert(blocking_level < 200);

  event_loop_inner();

  // Decrement level
  blocking_level -= 1;
}
