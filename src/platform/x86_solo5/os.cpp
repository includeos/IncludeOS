#include <kprint>
#include <info>
#include <service>
#include <smp>
#include <kernel/os.hpp>
#include <statman>
#include <kernel/rng.hpp>

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

RTC::timestamp_t OS::boot_timestamp()
{
  return booted_at_;
}

RTC::timestamp_t OS::uptime()
{
  return solo5_clock_monotonic() - booted_at_;
}

void OS::add_stdout_solo5()
{
  add_stdout(
  [] (const char* str, const size_t len) {
    solo5_console_write(str, len);
  });
}

void OS::start(char *cmdline, uintptr_t mem_size)
{
  PROFILE("");
  // Print a fancy header
  CAPTION("#include<os> // Literally");

  void* esp = get_cpu_esp();
  MYINFO("Stack: %p", esp);

  /// STATMAN ///
  /// initialize on page 7, 2 pages in size
  Statman::get().init(0x6000, 0x3000);

  PROFILE("Multiboot / legacy");

  low_memory_size_ = 0;
  high_memory_size_ = mem_size - 0x200000;

  Expects(high_memory_size_);

  PROFILE("Memory map");
  // Assign memory ranges used by the kernel
  auto& memmap = memory_map();

  OS::memory_end_ = high_memory_size_ + 0x100000;
  MYINFO("Assigning fixed memory ranges (Memory map)");

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

  PROFILE("Service::start");
  // begin service start
  FILLINE('=');
  printf(" IncludeOS %s (%s / %i-bit)\n",
         version().c_str(), arch().c_str(),
         static_cast<int>(sizeof(uintptr_t)) * 8);
  printf(" +--> Running [ %s ]\n", Service::name().c_str());
  FILLINE('~');

  Service::start();
  // NOTE: this is a feature for service writers, don't move!
  kernel_sanity_checks();
}

void OS::event_loop()
{
  printf("event_loop\n");
}

void OS::block()
{
  printf("block\n");
}
