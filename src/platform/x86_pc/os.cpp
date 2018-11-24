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

#define DEBUG
#define MYINFO(X,...) INFO("Kernel", X, ##__VA_ARGS__)

#include <boot/multiboot.h>
#include <kernel.hpp>
#include <os.hpp>
#include <rtc>
#include <kernel/events.hpp>
#include <kernel/memory.hpp>
#include <kprint>
#include <service>
#include <cstdio>
#include <cinttypes>
#include "cmos.hpp"

//#define ENABLE_PROFILERS
#ifdef ENABLE_PROFILERS
#include <profile>
#define PROFILE(name)  ScopedProfiler __CONCAT(sp, __COUNTER__){name};
#else
#define PROFILE(name) /* name */
#endif

extern "C" void* get_cpu_esp();
extern uintptr_t _start;
extern uintptr_t _end;
extern uintptr_t _ELF_START_;
extern uintptr_t _TEXT_START_;
extern uintptr_t _LOAD_START_;
extern uintptr_t _ELF_END_;
// in kernel/os.cpp
extern bool os_default_stdout;

struct alignas(SMP_ALIGN) OS_CPU {
  uint64_t cycles_hlt = 0;
};
static SMP::Array<OS_CPU> os_per_cpu;

uint64_t os::cycles_asleep() noexcept {
  return PER_CPU(os_per_cpu).cycles_hlt;
}
uint64_t os::nanos_asleep() noexcept {
  return (PER_CPU(os_per_cpu).cycles_hlt * 1e6) / os::cpu_freq().count();
}

__attribute__((noinline))
void os::halt() noexcept
{
  uint64_t cycles_before = os::Arch::cpu_cycles();
  asm volatile("hlt");

  // add a global symbol here so we can quickly discard
  // event loop from stack sampling
  asm volatile(
  ".global _irq_cb_return_location;\n"
  "_irq_cb_return_location:" );

  // Count sleep cycles
  PER_CPU(os_per_cpu).cycles_hlt += os::Arch::cpu_cycles() - cycles_before;
}

void kernel::default_stdout(const char* str, const size_t len)
{
  __serial_print(str, len);
}

extern kernel::ctor_t __stdout_ctors_start;
extern kernel::ctor_t __stdout_ctors_end;
extern void __arch_init_paging();
extern void __platform_init();
extern void elf_protect_symbol_areas();

void kernel::start(uint32_t boot_magic, uint32_t boot_addr)
{
  PROFILE("OS::start");
  kernel::state().cmdline = Service::binary_name();

  // Initialize stdout handlers
  if(os_default_stdout) {
    os::add_stdout(&kernel::default_stdout);
  }

  kernel::run_ctors(&__stdout_ctors_start, &__stdout_ctors_end);

  // Print a fancy header
  CAPTION("#include<os> // Literally");

  MYINFO("Stack: %p", get_cpu_esp());
  MYINFO("Boot magic: 0x%x, addr: 0x%x", boot_magic, boot_addr);

  // PAGING //
  PROFILE("Enable paging");
  __arch_init_paging();

  // BOOT METHOD //
  PROFILE("Multiboot / legacy");
  // Detect memory limits etc. depending on boot type
  if (boot_magic == MULTIBOOT_BOOTLOADER_MAGIC) {
    kernel::multiboot(boot_addr);
  } else {

    if (kernel::is_softreset_magic(boot_magic) && boot_addr != 0)
      kernel::resume_softreset(boot_addr);

    kernel::legacy_boot();
  }
  assert(kernel::memory_end() != 0);

  MYINFO("Total memory detected as %s ", util::Byte_r(kernel::memory_end()).to_string().c_str());

  // Give the rest of physical memory to heap
  kernel::state().heap_max = kernel::memory_end() - 1;
  assert(kernel::heap_begin() != 0x0 and kernel::heap_max() != 0x0);

  PROFILE("Memory map");
  // Assign memory ranges used by the kernel
  auto& memmap = os::mem::vmmap();
  INFO2("Assigning fixed memory ranges (Memory map)");
  // protect symbols early on (the calculation is complex so not doing it here)
  elf_protect_symbol_areas();

#if defined(ARCH_x86_64)
  // protect the basic pagetable used by LiveUpdate and any other
  // systems that need to exit long/protected mode
  os::mem::map({0x1000, 0x1000, os::mem::Access::read, 0x7000}, "Page tables");
  memmap.assign_range({0x10000, 0x9d3ff, "Stack"});
#elif defined(ARCH_i686)
  memmap.assign_range({0x10000, 0x9d3ff, "Stack"});
#endif

  // heap (physical) area
  uintptr_t span_max = std::numeric_limits<std::ptrdiff_t>::max();
  uintptr_t heap_range_max_ = std::min(span_max, kernel::heap_max());

  INFO2("* Assigning heap 0x%zx -> 0x%zx", kernel::heap_begin(), heap_range_max_);
  memmap.assign_range({kernel::heap_begin(), heap_range_max_,
        "Dynamic memory", kernel::heap_usage });

  MYINFO("Virtual memory map");
  for (const auto& entry : memmap)
      INFO2("%s", entry.second.to_string().c_str());

  PROFILE("Platform init");
  __platform_init();

  PROFILE("RTC init");
  // Realtime/monotonic clock
  RTC::init();
}

extern void __arch_poweroff();

void os::event_loop()
{
  Events::get(0).process_events();
  do {
    os::halt();
    Events::get(0).process_events();
  } while (kernel::is_running());

  MYINFO("Stopping service");
  Service::stop();

  MYINFO("Powering off");
  __arch_poweroff();
}


void kernel::legacy_boot()
{
  // Fetch CMOS memory info (unfortunately this is maximally 10^16 kb)
  auto mem = x86::CMOS::meminfo();
  if (kernel::memory_end() == os::Arch::max_canonical_addr)
  {
    //uintptr_t low_memory_size = mem.base.total * 1024;
    INFO2("* Low memory: %i Kib", mem.base.total);

    uintptr_t high_memory_size = mem.extended.total * 1024;
    INFO2("* High memory (from cmos): %i Kib", mem.extended.total);
    kernel::state().memory_end = 0x100000 + high_memory_size - 1;
  }

  auto& memmap = os::mem::vmmap();
  // No guarantees without multiboot, but we assume standard memory layout
  memmap.assign_range({0x0009FC00, 0x0009FFFF,
        "EBDA"});
  memmap.assign_range({0x000A0000, 0x000FFFFF,
        "VGA/ROM"});

}
