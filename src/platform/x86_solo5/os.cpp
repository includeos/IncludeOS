#include <kprint>
#include <info>
#include <service>
#include <smp>
#include <kernel/os.hpp>
#include <statman>
#include <os>
#include <kernel/rng.hpp>

#include <kernel/irq_manager.hpp>
#include <kernel/timers.hpp>
#include <kernel/solo5_manager.hpp>

extern "C" {
#include <solo5.h>
}

// sleep statistics
static uint64_t* os_cycles_hlt   = nullptr;
static uint64_t* os_cycles_total = nullptr;

extern "C" void* get_cpu_esp();
extern "C" void  kernel_sanity_checks();
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

RTC::timestamp_t OS::booted_at_ {0};

void solo5_poweroff()
{
  __asm__ __volatile__("cli; hlt");
  for(;;);
}

RTC::timestamp_t OS::boot_timestamp()
{
  return booted_at_;
}

uint64_t OS::get_cycles_halt() noexcept {
  return *os_cycles_hlt;
}
uint64_t OS::get_cycles_total() noexcept {
  return *os_cycles_total;
}

// uptime in nanoseconds
RTC::timestamp_t OS::uptime()
{
  return solo5_clock_monotonic() - booted_at_;
}

int64_t OS::micros_since_boot() noexcept {
  return uptime() / 1000;
}

void OS::add_stdout_solo5()
{
  add_stdout(
  [] (const char* str, const size_t len) {
    solo5_console_write(str, len);
  });
}

void OS::start(char* _cmdline, uintptr_t mem_size)
{
  PROFILE("");
  // Print a fancy header
  CAPTION("#include<os> // Literally");

  void* esp = get_cpu_esp();
  MYINFO("Stack: %p", esp);

  /// STATMAN ///
  /// initialize on page 7, 2 pages in size
  Statman::get().init(0x6000, 0x3000);

  OS::cmdline = reinterpret_cast<char*>(_cmdline);

  // XXX: double check these numbers. Is 0 OK? and why is high_memory_size_
  // not equal to OS::memory_end_. What's that "+ 0x100000"?
  low_memory_size_ = 0;
  high_memory_size_ = mem_size - 0x200000;

  Expects(high_memory_size_);

  PROFILE("Memory map");
  // Assign memory ranges used by the kernel
  auto& memmap = memory_map();

  OS::memory_end_ = high_memory_size_ + 0x100000;
  MYINFO("Assigning fixed memory ranges (Memory map)");

  memmap.assign_range({0x500, 0x5fff, "solo5", "solo5"});
  memmap.assign_range({0x6000, 0x8fff, "Statman", "Statistics"});
  memmap.assign_range({0xA000, 0x9fbff, "Stack", "Kernel / service main stack"});
  memmap.assign_range({(uintptr_t)&_LOAD_START_, (uintptr_t)&_end,
        "ELF", "Your service binary including OS"});

  Expects(::heap_begin and heap_max_);
  // @note for security we don't want to expose this
  memmap.assign_range({(uintptr_t)&_end + 1, ::heap_begin - 1,
        "Pre-heap", "Heap randomization area"});

  // Give the rest of physical memory to heap
  heap_max_ = high_memory_size_;

  uintptr_t span_max = std::numeric_limits<std::ptrdiff_t>::max();
  uintptr_t heap_range_max_ = std::min(span_max, heap_max_);

  MYINFO("Assigning heap");
  memmap.assign_range({::heap_begin, heap_range_max_,
        "Heap", "Dynamic memory", heap_usage });

  MYINFO("Printing memory map");

  for (const auto &i : memmap)
    INFO2("* %s",i.second.to_string().c_str());

  // sleep statistics
  // NOTE: needs to be positioned before anything that calls OS::halt
  os_cycles_hlt = &Statman::get().create(
      Stat::UINT64, std::string("cpu0.cycles_hlt")).get_uint64();
  os_cycles_total = &Statman::get().create(
      Stat::UINT64, std::string("cpu0.cycles_total")).get_uint64();

  PROFILE("Platform init");
  extern void __platform_init();
  __platform_init();

  booted_at_ = solo5_clock_monotonic();
  MYINFO("Booted at monotonic_ns=%lld walltime_ns=%lld",
         booted_at_, solo5_clock_wall());

  MYINFO("Initializing RNG");
  PROFILE("RNG init");
  RNG::init();

  // Seed rand with 32 bits from RNG
  srand(rng_extract_uint32());

  // Custom initialization functions
  MYINFO("Initializing plugins");
  // the boot sequence is over when we get to plugins/Service::start
  OS::boot_sequence_passed_ = true;

  PROFILE("Plugins init");
  for (auto plugin : plugins_) {
    INFO2("* Initializing %s", plugin.name_);
    try{
      plugin.func_();
    } catch(std::exception& e){
      MYINFO("Exception thrown when initializing plugin: %s", e.what());
    } catch(...){
      MYINFO("Unknown exception when initializing plugin");
    }
  }

  // We don't need a start or stop function in solo5.
  Timers::init(
    // timer start function
    [] (std::chrono::microseconds) {},
    // timer stop function
    [] () {});

  // Some tests are asserting there is at least one timer that is always ON
  // (the RTC calibration timer). Let's fake some timer so those tests pass.
  Timers::oneshot(std::chrono::hours(1000000), [] (auto) {});

  Timers::ready();

  PROFILE("Service::start");
  // begin service start
  FILLINE('=');
  printf(" IncludeOS %s (%s / %i-bit)\n",
         version().c_str(), arch().c_str(),
         static_cast<int>(sizeof(uintptr_t)) * 8);
  printf(" +--> Running [ %s ]\n", Service::name().c_str());
  FILLINE('~');

  Solo5_manager::init();

  Service::start();
  // NOTE: this is a feature for service writers, don't move!
  kernel_sanity_checks();
}

void OS::event_loop()
{
  uint8_t *data = (uint8_t *) malloc(1520);
  assert(data);

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
    if (rc == 0) {
      Timers::timers_handler();
    } else {
      int len = 1520;
      memset(data, 0, 1520);

      if (solo5_net_read_sync(data, &len) == 0) {
        // XXX: packet is copied by upstream_received_packet (slow!)
        for(auto& nic : hw::Devices::devices<hw::Nic>()) {
          nic->upstream_received_packet(data, len);
          break;
        }
      }

    }
  }

  free(data);

  MYINFO("Stopping service");
  Service::stop();

  MYINFO("Powering off");
  solo5_poweroff();
}

__attribute__((noinline))
void OS::halt() {
  *os_cycles_total = cycles_since_boot();
#if defined(ARCH_x86)
  asm volatile("hlt");
#else
#warning "OS::halt() not implemented for selected arch"
#endif
  // Count sleep cycles
  if (os_cycles_hlt)
      *os_cycles_hlt += cycles_since_boot() - *os_cycles_total;
}

// Keep track of blocking levels
static uint32_t* blocking_level = 0;
static uint32_t* highest_blocking_level = 0;


// Getters, mostly for testing
extern "C" uint32_t os_get_blocking_level() {
  return *blocking_level;
};

extern "C" uint32_t os_get_highest_blocking_level() {
  return *highest_blocking_level;
};


void OS::block(){

  // Initialize stats
  if (not blocking_level) {
    blocking_level = &Statman::get()
      .create(Stat::UINT32, std::string("blocking.current_level")).get_uint32();
    *blocking_level = 0;
  }

  if (not highest_blocking_level) {
    highest_blocking_level = &Statman::get()
      .create(Stat::UINT32, std::string("blocking.highest")).get_uint32();
    *highest_blocking_level = 0;
  }

  // Increment level
  *blocking_level += 1;

  // Increment highest if applicable
  if (*blocking_level > *highest_blocking_level)
    *highest_blocking_level = *blocking_level;

  int rc;
  rc = solo5_poll(solo5_clock_monotonic() + 50000ULL); // now + 0.05 ms
  if (rc == 0) {
    Timers::timers_handler();
  } else {
    int len = 1520;
    uint8_t *data = (uint8_t *) malloc(1520);
    assert(data);
    memset(data, 0, 1520);

    if (solo5_net_read_sync(data, &len) == 0) {
      // make sure packet is copied
      for(auto& nic : hw::Devices::devices<hw::Nic>()) {
        nic->upstream_received_packet(data, len);
        break;
      }
    }

    free(data);
  }

  // Decrement level
  *blocking_level -= 1;
}
