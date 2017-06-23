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

const std::string& OS::cmdline_args() noexcept
{
  return cmdline;
}

void OS::register_plugin(Plugin delg, const char* name){
  MYINFO("Registering plugin %s", name);
  plugins_.emplace(delg, name);
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
