#include <os>

#include <info>
#include <smp>
#include <statman>
#include <kernel/timers.hpp>
#include <kernel/solo5_manager.hpp>

extern "C" {
#include <solo5.h>
}

// sleep statistics
static uint64_t os_cycles_hlt = 0;

extern "C" void* get_cpu_esp();
extern "C" void __libc_init_array();
extern uintptr_t heap_begin;
extern uintptr_t heap_end;
extern uintptr_t _start;
extern uintptr_t _end;
extern uintptr_t mem_size;
extern uintptr_t _ELF_START_;
extern uintptr_t _TEXT_START_;
extern uintptr_t _LOAD_START_;
extern uintptr_t _ELF_END_;

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

void OS::start(char* _cmdline, uintptr_t mem_size)
{
  // Initialize stdout handlers
  OS::add_stdout(&OS::default_stdout);

  PROFILE("");
  // Print a fancy header
  CAPTION("#include<os> // Literally");

  void* esp = get_cpu_esp();
  MYINFO("Stack: %p", esp);

  /// STATMAN ///
  /// initialize on page 7, 2 pages in size
  Statman::get().init(0x6000, 0x3000);

  OS::cmdline = _cmdline;

  // setup memory and heap end
  OS::memory_end_ = mem_size;
  OS::heap_max_ = OS::memory_end_;

  // Call global ctors
  PROFILE("Global constructors");
  __libc_init_array();


  PROFILE("Memory map");
  // Assign memory ranges used by the kernel
  auto& memmap = memory_map();
  MYINFO("Assigning fixed memory ranges (Memory map)");

  memmap.assign_range({0x500, 0x5fff, "solo5"});
  memmap.assign_range({0x6000, 0x8fff, "Statman"});
  memmap.assign_range({0xA000, 0x9fbff, "Stack"});
  memmap.assign_range({(uintptr_t)&_LOAD_START_, (uintptr_t)&_end,
        "ELF"});

  Expects(::heap_begin and heap_max_);
  // @note for security we don't want to expose this
  memmap.assign_range({(uintptr_t)&_end + 1, ::heap_begin - 1,
        "Pre-heap"});

  uintptr_t span_max = std::numeric_limits<std::ptrdiff_t>::max();
  uintptr_t heap_range_max_ = std::min(span_max, heap_max_);

  MYINFO("Assigning heap");
  memmap.assign_range({::heap_begin, heap_range_max_,
        "Dynamic memory", heap_usage });

  MYINFO("Printing memory map");
  for (const auto &i : memmap)
    INFO2("* %s",i.second.to_string().c_str());

  extern void __platform_init();
  __platform_init();

  MYINFO("Booted at monotonic_ns=%ld walltime_ns=%ld",
         solo5_clock_monotonic(), solo5_clock_wall());

  Solo5_manager::init();

  // We don't need a start or stop function in solo5.
  Timers::init(
    // timer start function
    [] (auto) {},
    // timer stop function
    [] () {});

  Timers::ready();
}

void OS::event_loop()
{
  while (power_) {
    int rc;

    // add a global symbol here so we can quickly discard
    // event loop from stack sampling
    asm volatile(
    ".global _irq_cb_return_location;\n"
    "_irq_cb_return_location:" );

    // XXX: temporarily ALWAYS sleep for 0.5 ms. We should ideally ask Timers
    // for the next immediate timer to fire (the first from the "scheduled" list
    // of timers?)
    rc = solo5_poll(solo5_clock_monotonic() + 500000ULL); // now + 0.5 ms
    Timers::timers_handler();
    if (rc) {
      for(auto& nic : hw::Devices::devices<hw::Nic>()) {
        nic->poll();
        break;
      }
    }
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

// Keep track of blocking levels
static uint32_t blocking_level = 0;
static uint32_t highest_blocking_level = 0;

void OS::block()
{
  // Increment level
  blocking_level += 1;

  // Increment highest if applicable
  if (blocking_level > highest_blocking_level)
      highest_blocking_level = blocking_level;

  int rc;
  rc = solo5_poll(solo5_clock_monotonic() + 50000ULL); // now + 0.05 ms
  if (rc == 0) {
    Timers::timers_handler();
  } else {

    for(auto& nic : hw::Devices::devices<hw::Nic>()) {
      nic->poll();
      break;
    }

  }

  // Decrement level
  blocking_level -= 1;
}

extern "C"
void __os_store_soft_reset(void*, size_t) {}
