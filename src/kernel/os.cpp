// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2017 Oslo and Akershus University College of Applied Sciences
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
#include <boot/multiboot.h>
#include <hw/cmos.hpp>
#include <kernel/os.hpp>
#include <kernel/irq_manager.hpp>
#include <kernel/rtc.hpp>
#include <kernel/rdrand.hpp>
#include <kernel/rng.hpp>
#include <kernel/cpuid.hpp>
#include <util/fixedvec.hpp>
#include <kprint>
#include <service>
#include <statman>
#include <cinttypes>

//#define ENABLE_PROFILERS
#ifdef ENABLE_PROFILERS
#include <profile>
#define PROFILE(name)  ScopedProfiler __CONCAT(sp, __COUNTER__){name};
#else
#define PROFILE(name) /* name */
#endif

extern "C" void* get_cpu_esp();
extern "C" void  kernel_sanity_checks();
extern uintptr_t heap_begin;
extern uintptr_t heap_end;
extern uintptr_t _start;
extern uintptr_t _end;
extern uintptr_t _ELF_START_;
extern uintptr_t _TEXT_START_;
extern uintptr_t _LOAD_START_;
extern uintptr_t _ELF_END_;

// Initialize static OS data members
bool  OS::power_   = true;
bool  OS::boot_sequence_passed_ = false;
MHz   OS::cpu_mhz_ {-1};
uintptr_t OS::low_memory_size_  {0};
uintptr_t OS::high_memory_size_ {0};
uintptr_t OS::memory_end_ {0};
uintptr_t OS::heap_max_ {0xfffffff};
const uintptr_t OS::elf_binary_size_ {(uintptr_t)&_ELF_END_ - (uintptr_t)&_ELF_START_};
std::string OS::cmdline{Service::binary_name()};

// stdout redirection
using Print_vec = fixedvector<OS::print_func, 8>;
static Print_vec os_print_handlers(Fixedvector_Init::UNINIT);
extern void default_stdout_handlers();

// Plugins
OS::Plugin_vec OS::plugins_(Fixedvector_Init::UNINIT);

// OS version
#ifndef OS_VERSION
#define OS_VERSION "v?.?.?"
#endif
std::string OS::version_str_ = OS_VERSION;
std::string OS::arch_str_ = ARCH;

// sleep statistics
static uint64_t* os_cycles_hlt   = nullptr;
static uint64_t* os_cycles_total = nullptr;

const std::string& OS::cmdline_args() noexcept
{
  return cmdline;
}

void OS::start(uint32_t boot_magic, uint32_t boot_addr)
{
  PROFILE("");
  // Print a fancy header
  CAPTION("#include<os> // Literally");

  void* esp = get_cpu_esp();
  MYINFO("Stack: %p", esp);
  MYINFO("Boot magic: 0x%x, addr: 0x%x",
         boot_magic, boot_addr);

  /// STATMAN ///
  /// initialize on page 7, 2 pages in size
  Statman::get().init(0x6000, 0x2000);

  PROFILE("Multiboot / legacy");
  // Detect memory limits etc. depending on boot type
  if (boot_magic == MULTIBOOT_BOOTLOADER_MAGIC) {
    OS::multiboot(boot_addr);
  } else {

    if (is_softreset_magic(boot_magic) && boot_addr != 0)
        OS::resume_softreset(boot_addr);

    OS::legacy_boot();
  }
  Expects(high_memory_size_);

  PROFILE("Memory map");
  // Assign memory ranges used by the kernel
  auto& memmap = memory_map();

  OS::memory_end_ = high_memory_size_ + 0x100000;
  MYINFO("Assigning fixed memory ranges (Memory map)");

  memmap.assign_range({0x6000, 0x7fff, "Statman", "Statistics"});
  memmap.assign_range({0xA000, 0x9fbff, "Stack", "Kernel / service main stack"});
  memmap.assign_range({(uintptr_t)&_LOAD_START_, (uintptr_t)&_end,
        "ELF", "Your service binary including OS"});

  Expects(::heap_begin and heap_max_);
  // @note for security we don't want to expose this
  memmap.assign_range({(uintptr_t)&_end + 1, ::heap_begin - 1,
        "Pre-heap", "Heap randomization area"});

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


  // sleep statistics
  // NOTE: needs to be positioned before anything that calls OS::halt
  os_cycles_hlt = &Statman::get().create(
      Stat::UINT64, std::string("cpu0.cycles_hlt")).get_uint64();
  os_cycles_total = &Statman::get().create(
      Stat::UINT64, std::string("cpu0.cycles_total")).get_uint64();

  PROFILE("Platform init");
  extern void __platform_init();
  __platform_init();
  kernel_sanity_checks();

  PROFILE("RTC init");
  // Realtime/monotonic clock
  RTC::init();
  kernel_sanity_checks();

  MYINFO("Initializing RNG");
  PROFILE("RNG init");
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
}

void OS::register_plugin(Plugin delg, const char* name){
  MYINFO("Registering plugin %s", name);
  plugins_.emplace(delg, name);
}

int64_t OS::micros_since_boot() noexcept {
  return cycles_since_boot() / cpu_freq().count();
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
#if defined(ARCH_x86)
  asm volatile("hlt");

  // add a global symbol here so we can quickly discard
  // event loop from stack sampling
  asm volatile(
  ".global _irq_cb_return_location;\n"
  "_irq_cb_return_location:" );
#else
#warning "OS::halt() not implemented for selected arch"
#endif
  // Count sleep cycles
  *os_cycles_hlt += cycles_since_boot() - *os_cycles_total;
}

void OS::event_loop()
{

  IRQ_manager::get().process_interrupts();
  do {
    OS::halt();
    IRQ_manager::get().process_interrupts();
  } while (power_);

  MYINFO("Stopping service");
  Service::stop();

  MYINFO("Powering off");
  extern void __arch_poweroff();
  __arch_poweroff();
}

void OS::reboot()
{
  extern void __arch_reboot();
  __arch_reboot();
}
void OS::shutdown()
{
  MYINFO("Soft shutdown signalled");
  power_ = false;
}

void OS::add_stdout(OS::print_func func)
{
  os_print_handlers.add(func);
}
void OS::add_stdout_default_serial()
{
  add_stdout(
  [] (const char* str, const size_t len) {
    kprintf("%.*s", static_cast<int>(len), str);
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

void OS::legacy_boot() {
  // Fetch CMOS memory info (unfortunately this is maximally 10^16 kb)
  auto mem = hw::CMOS::meminfo();
  low_memory_size_ = mem.base.total * 1024;
  INFO2("* Low memory: %i Kib", mem.base.total);
  high_memory_size_ = mem.extended.total * 1024;

  // Use memsize provided by Make / linker unless CMOS knows this is wrong
  INFO2("* High memory (from cmos): %i Kib", mem.extended.total);

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
    INFO2("* Unavailable memory: 0x%" PRIxPTR" - 0x%" PRIxPTR, unavail_start, unavail_end);
    memmap.assign_range({unavail_start, unavail_end,
          "N/A", "Reserved / outside physical range" });
    unavail_start = unavail_end + 1;
    interval = std::min(span_max, addr_max - unavail_start);
    // Increment might wrapped around
    if (unavail_start > unavail_end + interval or unavail_start + interval == addr_max){
      INFO2("* Last chunk of memory: 0x%" PRIxPTR" - 0x%" PRIxPTR, unavail_start, addr_max);
      memmap.assign_range({unavail_start, addr_max,
            "N/A", "Reserved / outside physical range" });
      break;
    }

    unavail_end += interval;
  }
}
