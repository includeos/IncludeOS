// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//#define DEBUG
#define MYINFO(X,...) INFO("Kernel", X, ##__VA_ARGS__)

#include <cstdio>
#include <os>
#include <boot/multiboot.h>
#include <kernel/elf.hpp>
#include <hw/acpi.hpp>
#include <hw/apic.hpp>
#include <hw/apic_timer.hpp>
#include <hw/cmos.hpp>
#include <kernel/irq_manager.hpp>
#include <kernel/pci_manager.hpp>
#include <kernel/timers.hpp>
#include <kernel/rtc.hpp>
#include <kernel/rdrand.hpp>
#include <kernel/rng.hpp>
#include <kernel/cpuid.hpp>
#include <kprint>
#include <statman>
#include <vector>

extern "C" void kernel_sanity_checks();
//#define ENABLE_PROFILERS

#ifdef ENABLE_PROFILERS
#include <profile>
#endif

extern "C" uint16_t _cpu_sampling_freq_divider_;
extern uintptr_t heap_begin;
extern uintptr_t heap_end;
extern uintptr_t _start;
extern uintptr_t _end;
extern uintptr_t _ELF_START_;
extern uintptr_t _TEXT_START_;
extern uintptr_t _LOAD_START_;
extern uintptr_t _ELF_END_;
extern uintptr_t _MAX_MEM_MIB_;

bool  OS::power_   = true;
MHz   OS::cpu_mhz_ {-1};
RTC::timestamp_t OS::booted_at_ {0};
uintptr_t OS::low_memory_size_  {0};
uintptr_t OS::high_memory_size_ {0};
uintptr_t OS::memory_end_ {0};
uintptr_t OS::heap_max_ {0xfffffff};
const uintptr_t OS::elf_binary_size_ {(uintptr_t)&_ELF_END_ - (uintptr_t)&_ELF_START_};
// stdout redirection
static std::vector<OS::print_func> os_print_handlers;
extern void default_stdout_handlers();
// custom init
std::vector<OS::Plugin_struct> OS::plugins_;
// OS version
#ifndef OS_VERSION
#define OS_VERSION "v?.?.?"
#endif
std::string OS::version_field = OS_VERSION;

// Multiboot command line for the service
static std::string os_cmdline = Service::binary_name();
// sleep statistics
static uint64_t* os_cycles_hlt   = nullptr;
static uint64_t* os_cycles_total = nullptr;
extern "C" uintptr_t get_cpu_esp();

const std::string& OS::cmdline_args() noexcept
{
  return os_cmdline;
}

void OS::start(uint32_t boot_magic, uint32_t boot_addr) {

#ifdef ENABLE_PROFILERS
  ScopedProfiler sp1{};
#endif

  default_stdout_handlers();

  // Print a fancy header
  CAPTION("#include<os> // Literally");

  auto esp = get_cpu_esp();
  MYINFO ("Stack: 0x%x", esp);
  Expects (esp < 0xA0000 and esp > 0x0 and "Stack location OK");

  MYINFO("Boot args: 0x%x (multiboot magic), 0x%x (bootinfo addr)",
         boot_magic, boot_addr);

  MYINFO("Max mem (from linker): %i MiB", reinterpret_cast<size_t>(&_MAX_MEM_MIB_));


  // Detect memory limits etc. depending on boot type
  if (boot_magic == MULTIBOOT_BOOTLOADER_MAGIC) {
    OS::multiboot(boot_magic, boot_addr);
  } else {

    if (is_softreset_magic(boot_magic) && boot_addr != 0)
        OS::resume_softreset(boot_addr);

    OS::legacy_boot();
  }

  Expects(high_memory_size_);

  // Assign memory ranges used by the kernel
  auto& memmap = memory_map();

  OS::memory_end_ = high_memory_size_ + 0x100000;

  MYINFO("Assigning fixed memory ranges (Memory map)");

  memmap.assign_range({0x4000, 0x5fff, "Statman", "Statistics"});
  memmap.assign_range({0xA000, 0x9fbff, "Kernel / service main stack"});
  memmap.assign_range({(uintptr_t)&_LOAD_START_, (uintptr_t)&_end,
        "ELF", "Your service binary including OS"});

  Expects(::heap_begin and heap_max_);
  // @note for security we don't want to expose this
  memmap.assign_range({(uintptr_t)&_end + 1, ::heap_begin - 1,
        "Pre-heap", "Heap randomization area (not for use))"});

  // Give the rest of physical memory to heap
  heap_max_ = ((0x100000 + high_memory_size_)  & 0xffff0000) - 1;

  uintptr_t span_max = std::numeric_limits<std::ptrdiff_t>::max();
  uintptr_t heap_range_max_ = std::min(span_max, heap_max_);

  MYINFO("Assigning heap");
  memmap.assign_range({::heap_begin, heap_range_max_,
        "Heap", "Dynamic memory", heap_usage });

  MYINFO("Printing memory map");

  for (const auto &i : memmap)
    INFO2("* %s",i.second.to_string().c_str());

#ifdef ENABLE_PROFILERS
  ScopedProfiler sp2("OS::start IRQ manager init");
#endif
  // Set up interrupt and exception handlers
  IRQ_manager::init();

#ifdef ENABLE_PROFILERS
  ScopedProfiler sp3("OS::start ACPI init");
#endif
  // read ACPI tables
  hw::ACPI::init();

#ifdef ENABLE_PROFILERS
  ScopedProfiler sp4("OS::start APIC init");
#endif
  // setup APIC, APIC timer, SMP etc.
  hw::APIC::init();

  // enable interrupts
  INFO("BSP", "Enabling interrupts");
  IRQ_manager::enable_interrupts();

#ifdef ENABLE_PROFILERS
  ScopedProfiler sp5("OS::start PIT init");
#endif
  // Initialize the Interval Timer
  hw::PIT::init();

#ifdef ENABLE_PROFILERS
  ScopedProfiler sp6("OS::start PCI manager init");
#endif
  // Initialize PCI devices
  PCI_manager::init();

  // Print registered devices
  hw::Devices::print_devices();

  // sleep statistics
  // NOTE: needs to be positioned before anything that calls OS::halt
  os_cycles_hlt = &Statman::get().create(
      Stat::UINT64, std::string("cpu0.cycles_hlt")).get_uint64();
  os_cycles_total = &Statman::get().create(
      Stat::UINT64, std::string("cpu0.cycles_total")).get_uint64();

  // Estimate CPU frequency
  MYINFO("Estimating CPU-frequency");
  INFO2("|");
  INFO2("+--(2 samples, %f sec. interval)",
        (hw::PIT::frequency() / _cpu_sampling_freq_divider_).count());
  INFO2("|");

#ifdef ENABLE_PROFILERS
  ScopedProfiler sp7("OS::start CPU frequency");
#endif
  // TODO: Debug why actual measurments sometimes causes problems. Issue #246.
  if (OS::cpu_mhz_.count() < 0) {
    OS::cpu_mhz_ = MHz(hw::PIT::estimate_CPU_frequency(16));
  }
  INFO2("+--> %f MHz", cpu_freq().count());

#ifdef ENABLE_PROFILERS
  ScopedProfiler sp8("OS::start Timers init");
#endif
  // cpu_mhz must be known before we can start timer system
  /// initialize timers hooked up to APIC timer
  Timers::init(
    // timer start function
    hw::APIC_Timer::oneshot,
    // timer stop function
    hw::APIC_Timer::stop);

  // initialize BSP APIC timer
  hw::APIC_Timer::init(
  [] {
    // set final interrupt handler
    hw::APIC_Timer::set_handler(Timers::timers_handler);
    // signal that kernel is done with everything
    Service::ready();
    // signal ready
    // NOTE: this executes the first timers, so we
    // don't want to run this before calling Service ready
    Timers::ready();
  });

#ifdef ENABLE_PROFILERS
  ScopedProfiler sp9("OS::start RTC init");
#endif
  // Realtime/monotonic clock
  RTC::init();
  booted_at_ = RTC::now();

#ifdef ENABLE_PROFILERS
  ScopedProfiler sp10("OS::start Plugins init");
#endif
  // Trying custom initialization functions
  MYINFO("Initializing plugins");
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

#ifdef ENABLE_PROFILERS
  ScopedProfiler sp11("OS::start RNG init");
#endif
  // initialize random seed based on cycles since start
  if (CPUID::has_feature(CPUID::Feature::RDRAND)) {
    uint32_t rdrand_output[32];

    for (size_t i = 0; i != 32; ++i) {
      while (!rdrand32(&rdrand_output[i])) {}
    }

    rng_absorb(rdrand_output, sizeof(rdrand_output));
  }
  else {
    // this is horrible, better solution needed here
   for (size_t i = 0; i != 32; ++i) {
      uint64_t clock = cycles_since_boot();
      // maybe additionally call something which will take
      // variable time depending in some way on the processor
      // state (clflush?) or a HAVEGE-like approach.
      rng_absorb(&clock, sizeof(clock));
    }
  }

  // Seed rand with 32 bits from RNG
  srand(rng_extract_uint32());

  // Everything is ready
  MYINFO("Starting %s", Service::name().c_str());
  FILLINE('=');

  // begin service start
  Service::start();
}

void OS::register_plugin(Plugin delg, const char* name){
  MYINFO("Registering plugin %s", name);
  plugins_.emplace_back(delg, name);
}

uintptr_t OS::heap_begin() noexcept
{
  return ::heap_begin;
}
uintptr_t OS::heap_end() noexcept
{
  return ::heap_end;
}

uintptr_t OS::resize_heap(size_t size){

  uintptr_t new_end = heap_begin() + size;
  if (not size or size < heap_usage() or new_end > memory_end())
    return heap_max() - heap_begin();

  memory_map().resize(heap_begin(), size);
  heap_max_ = heap_begin() + size;
  return size;
}

uint64_t OS::get_cycles_halt() noexcept {
  return *os_cycles_hlt;
}
uint64_t OS::get_cycles_total() noexcept {
  return *os_cycles_total;
}

__attribute__((noinline))
void OS::halt() {
  *os_cycles_total = cycles_since_boot();
  asm volatile("hlt");

  // add a global symbol here so we can quickly discard
  // event loop from stack sampling
  asm volatile(
  ".global _irq_cb_return_location;\n"
  "_irq_cb_return_location:" );

  // Count sleep cycles
  *os_cycles_hlt += cycles_since_boot() - *os_cycles_total;
}

void OS::event_loop() {
  FILLINE('=');
  printf(" IncludeOS %s\n", version().c_str());
  printf(" +--> Running [ %s ]\n", Service::name().c_str());
  FILLINE('~');

  while (power_) {
    IRQ_manager::get().process_interrupts();
    debug2("OS going to sleep.\n");
    OS::halt();
  }

  // Cleanup
  Service::stop();
  /// TODO: move me to arch-specific module
  hw::ACPI::shutdown();
}

void OS::reboot()
{
  /// TODO: move me to arch-specific module
  hw::ACPI::reboot();
}
void OS::shutdown()
{
  power_ = false;
}
void OS::on_panic(on_panic_func func)
{
  extern on_panic_func panic_handler;
  panic_handler = func;
}

void OS::add_stdout(OS::print_func func)
{
  os_print_handlers.push_back(func);
}
void OS::add_stdout_default_serial()
{
  add_stdout(
  [] (const char* str, const size_t len) {
    kprintf("%.*s", len, str);
  });
}
__attribute__ ((weak))
void default_stdout_handlers()
{
  OS::add_stdout_default_serial();
}
size_t OS::print(const char* str, const size_t len)
{
  for (auto& func : os_print_handlers)
      func(str, len);
  return len;
}

void OS::multiboot(uint32_t boot_magic, uint32_t boot_addr){
  MYINFO("Booted with multiboot");
  INFO2("* magic value: 0x%x Multiboot info at 0x%x", boot_magic, boot_addr);

  multiboot_info_t* bootinfo = (multiboot_info_t*) boot_addr;

  if (! bootinfo->flags & MULTIBOOT_INFO_MEMORY) {
    INFO2("* No memory info provided in multiboot info");
    return;
  }

  uint32_t mem_low_start = 0;
  uint32_t mem_low_end = (bootinfo->mem_lower * 1024) - 1;
  uint32_t mem_low_kb = bootinfo->mem_lower;
  uint32_t mem_high_start = 0x100000;
  uint32_t mem_high_end = mem_high_start + (bootinfo->mem_upper * 1024) - 1;
  uint32_t mem_high_kb = bootinfo->mem_upper;

  OS::low_memory_size_ = mem_low_kb * 1024;
  OS::high_memory_size_ = mem_high_kb * 1024;
  OS::memory_end_ = high_memory_size_ + mem_high_start;

  INFO2("* Valid memory (%i Kib):", mem_low_kb + mem_high_kb);
  INFO2("\t 0x%08x - 0x%08x (%i Kib)",
        mem_low_start, mem_low_end, mem_low_kb);
  INFO2("\t 0x%08x - 0x%08x (%i Kib)",
        mem_high_start, mem_high_end, mem_high_kb);
  INFO2("");

  if (bootinfo->flags & MULTIBOOT_INFO_CMDLINE) {
    INFO2("* Booted with parameters @ %p: %s",(void*)bootinfo->cmdline, (char*)bootinfo->cmdline);
    os_cmdline = (char*) bootinfo->cmdline;

  }

  if (bootinfo->flags & MULTIBOOT_INFO_MEM_MAP) {
    INFO2("* Multiboot provided memory map  (%i entries @ %p)",
          bootinfo->mmap_length / sizeof(multiboot_memory_map_t), (void*)bootinfo->mmap_addr);
    gsl::span<multiboot_memory_map_t> mmap { reinterpret_cast<multiboot_memory_map_t*>(bootinfo->mmap_addr),
        (int)(bootinfo->mmap_length / sizeof(multiboot_memory_map_t))};

    for (auto map : mmap) {
      const char* str_type = map.type & MULTIBOOT_MEMORY_AVAILABLE ? "FREE" : "RESERVED";
      INFO2("\t 0x%08llx - 0x%08llx %s (%llu Kb.)",
            map.addr, map.addr + map.len - 1, str_type, map.len / 1024 );

      if (not (map.type & MULTIBOOT_MEMORY_AVAILABLE)) {
        memory_map().assign_range({static_cast<uintptr_t>(map.addr), static_cast<uintptr_t>(map.addr + map.len - 1), "Reserved", "Multiboot / BIOS"});
      }
    }
    printf("\n");
  }
}


void OS::legacy_boot() {
  // Fetch CMOS memory info (unfortunately this is maximally 10^16 kb)
  auto mem = cmos::meminfo();
  low_memory_size_ = mem.base.total * 1024;
  INFO2("* Low memory: %i Kib", mem.base.total);
  high_memory_size_ = mem.extended.total * 1024;

  // Use memsize provided by Make / linker unless CMOS knows this is wrong
  decltype(high_memory_size_) hardcoded_mem = reinterpret_cast<size_t>(&_MAX_MEM_MIB_ - 0x100000) << 20;
  if (mem.extended.total == 0xffff or hardcoded_mem < mem.extended.total) {
    high_memory_size_ = hardcoded_mem;
    INFO2("* High memory (from linker): %i Kib", high_memory_size_ / 1024);
  } else {
    INFO2("* High memory (from cmos): %i Kib", mem.extended.total);
  }

  auto& memmap = memory_map();

  // No guarantees without multiboot, but we assume standard memory layout
  memmap.assign_range({0x0009FC00, 0x0009FFFF,
        "EBDA", "Extended BIOS data area"});
  memmap.assign_range({0x000A0000, 0x000FFFFF,
        "VGA/ROM", "Memory mapped video memory"});

  // @note : since the maximum size of a span is unsigned (ptrdiff_t) we may need more than one
  uintptr_t addr_max = std::numeric_limits<std::size_t>::max();
  uintptr_t span_max = std::numeric_limits<std::ptrdiff_t>::max();

  uintptr_t unavail_start = 0x100000 + high_memory_size_;
  size_t interval = std::min(span_max, addr_max - unavail_start) - 1;
  uintptr_t unavail_end = unavail_start + interval;

  while (unavail_end < addr_max){
    INFO2("* Unavailable memory: 0x%x - 0x%x", unavail_start, unavail_end);
    memmap.assign_range({unavail_start, unavail_end,
          "N/A", "Reserved / outside physical range" });
    unavail_start = unavail_end + 1;
    interval = std::min(span_max, addr_max - unavail_start);
    // Increment might wrapped around
    if (unavail_start > unavail_end + interval or unavail_start + interval == addr_max){
      INFO2("* Last chunk of memory: 0x%x - 0x%x", unavail_start, addr_max);
      memmap.assign_range({unavail_start, addr_max,
            "N/A", "Reserved / outside physical range" });
      break;
    }

    unavail_end += interval;
  }
}
