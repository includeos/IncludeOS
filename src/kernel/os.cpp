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
#include <hw/serial.hpp>
#include <kernel/irq_manager.hpp>
#include <kernel/pci_manager.hpp>
#include <kernel/timers.hpp>
#include <kernel/rtc.hpp>
#include <vector>

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

bool OS::power_   {true};
MHz  OS::cpu_mhz_ {1000};
RTC::timestamp_t OS::booted_at_ {0};
uintptr_t OS::low_memory_size_ {0};
uintptr_t OS::high_memory_size_ {0};
uintptr_t OS::heap_max_ {0xfffffff};
const uintptr_t OS::elf_binary_size_ {(uintptr_t)&_ELF_END_ - (uintptr_t)&_ELF_START_};

std::vector<OS::Custom_init_struct> OS::custom_init_;

#ifndef OS_VERSION
#define OS_VERSION "v?.?.?"
#endif
std::string OS::version_field = OS_VERSION;

// Set default rsprint_handler
OS::rsprint_func OS::rsprint_handler_ = &OS::default_rsprint;
hw::Serial& OS::com1 = hw::Serial::port<1>();
// Multiboot command line for the service
static std::string os_cmdline = "";

void OS::start(uint32_t boot_magic, uint32_t boot_addr) {

  // Print a fancy header
  FILLINE('=');
  CAPTION("#include<os> // Literally\n");
  FILLINE('=');

  uintptr_t esp;
  asm ("mov %%esp, %0"
       : "=r"(esp));
  MYINFO ("Stack: 0x%x", esp);
  Expects (esp < (uintptr_t)&_LOAD_START_ and esp >= 0x100000 and "Stack location OK");

  MYINFO("Boot args: 0x%x (multiboot magic), 0x%x (bootinfo addr)",
         boot_magic, boot_addr);

  MYINFO("Max mem (from linker): %i MiB", reinterpret_cast<size_t>(&_MAX_MEM_MIB_));

  if (boot_magic == MULTIBOOT_BOOTLOADER_MAGIC) {
    OS::multiboot(boot_magic, boot_addr);
  } else {

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
  }

  debug("\t[*] OS class started\n");
  srand((uint32_t) cycles_since_boot());

  atexit(default_exit);

  MYINFO("Assigning fixed memory ranges (Memory map)");

  auto& memmap = memory_map();

  // @ Todo: The first ~600k of memory is free for use. What can we put there?
  memmap.assign_range({0x0009FC00, 0x0009FFFF,
        "EBDA", "Extended BIOS data area"});
  memmap.assign_range({0x000A0000, 0x000FFFFF,
        "VGA/ROM", "Memory mapped video memory"});
  memmap.assign_range({0x00100000, (uintptr_t)&_LOAD_START_ -1 ,
        "Stack", "Kernel / service main stack"});
  memmap.assign_range({(uintptr_t)&_LOAD_START_, (uintptr_t)&_end,
        "ELF", "Your service binary including OS"});

  // @note for security we don't want to expose this
  memmap.assign_range({(uintptr_t)&_end + 1, heap_begin - 1,
        "Pre-heap", "Heap randomization area (not for use))"});

  memmap.assign_range({0x8000, 0x9fff, "Statman", "Statistics"});
  memmap.assign_range({0xA000, 0x9fbff, "Symbols", "ELF symbol/string sections"});

  // Create ranges for heap and the remaining address space
  // @note : since the maximum size of a span is unsigned (ptrdiff_t) we may need more than one
  uintptr_t addr_max = std::numeric_limits<std::size_t>::max();
  uintptr_t span_max = std::numeric_limits<std::ptrdiff_t>::max();

  // Give the rest of physical memory to heap
  heap_max_ = ((0x100000 + high_memory_size_)  & 0xffff0000) - 1;

  // ...Unless it's more than the maximum for a range
  // @note : this is a stupid way to limit the heap - we'll change it, but not until
  // we have a good solution.
  heap_max_ = std::min(span_max, heap_max_);

  memmap.assign_range({heap_begin, heap_max_,
        "Heap", "Dynamic memory", heap_usage });

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


  MYINFO("Printing memory map");

  for (const auto &i : memory_map())
    INFO2("* %s",i.second.to_string().c_str());

  // Set up interrupt and exception handlers
  IRQ_manager::init();

  // read ACPI tables
  hw::ACPI::init();

  // setup APIC, APIC timer, SMP etc.
  hw::APIC::init();

  // enable interrupts
  INFO("BSP", "Enabling interrupts");
  IRQ_manager::enable_interrupts();

  // Initialize the Interval Timer
  hw::PIT::init();

  // Initialize PCI devices
  PCI_manager::init();

  // Print registered devices
  hw::Devices::print_devices();

  // Estimate CPU frequency
  MYINFO("Estimating CPU-frequency");
  INFO2("|");
  INFO2("+--(10 samples, %f sec. interval)",
        (hw::PIT::frequency() / _cpu_sampling_freq_divider_).count());
  INFO2("|");

  // TODO: Debug why actual measurments sometimes causes problems. Issue #246.
  cpu_mhz_ = hw::PIT::CPU_frequency();
  INFO2("+--> %f MHz", cpu_mhz_.count());

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

  // Realtime/monotonic clock
  RTC::init();
  booted_at_ = RTC::now();

  // Trying custom initialization functions
  MYINFO("Calling custom initialization functions");
  for (auto init : custom_init_) {
    INFO2("* Calling %s", init.name_);
    try{
      init.func_();
    } catch(std::exception& e){
      MYINFO("Exception thrown when calling custom init: %s", e.what());
    } catch(...){
      MYINFO("Unknown exception when calling custom initialization function");
    }
  }
  // Everything is ready
  MYINFO("Starting %s", Service::name().c_str());
  FILLINE('=');
  Service::start(Service::command_line());

  event_loop();
}

void OS::register_custom_init(Custom_init delg, const char* name){
  MYINFO("Registering custom init function %s", name);
  custom_init_.emplace_back(delg, name);
}

uintptr_t OS::heap_max() {
  // Before the memory map is populated
  if (UNLIKELY(memory_map().empty()))
    return heap_max_;

  // After memory map is populated
  return memory_map().at(heap_begin).addr_end();
}

uintptr_t OS::heap_usage() {
  // measures heap usage only?
  return (uint32_t) (heap_end - heap_begin);
}

void OS::halt() {
  __asm__ volatile("hlt;");
}

void OS::event_loop() {
  FILLINE('=');
  printf(" IncludeOS %s\n", version().c_str());
  printf(" +--> Running [ %s ]\n", Service::name().c_str());
  FILLINE('~');

  while (power_) {
    IRQ_manager::get().notify();
  }

  // Cleanup
  Service::stop();
  // ACPI shutdown sequence
  hw::ACPI::shutdown();
}

void OS::shutdown()
{
  power_ = false;
}

size_t OS::rsprint(const char* str) {
  size_t len = 0;

  // Measure length
  while (str[len++]);

  // Output callback
  rsprint_handler_(str, len);
  return len;
}

size_t OS::rsprint(const char* str, const size_t len) {
  // Output callback
  OS::rsprint_handler_(str, len);
  return len;
}

void OS::default_rsprint(const char* str, size_t len) {
  for(size_t i = 0; i < len; ++i)
    com1.write(str[i]);
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

  INFO2("* Valid memory (%i Kib):", mem_low_kb + mem_high_kb);
  INFO2("\t 0x%08x - 0x%08x (%i Kib)",
        mem_low_start, mem_low_end, mem_low_kb);
  INFO2("\t 0x%08x - 0x%08x (%i Kib)",
        mem_high_start, mem_high_end, mem_high_kb);
  INFO2("");

  if (bootinfo->flags & MULTIBOOT_INFO_CMDLINE) {
    os_cmdline = (char*) bootinfo->cmdline;
    INFO2("* Booted with parameters: %s", os_cmdline.c_str());
  }

  if (bootinfo->flags & MULTIBOOT_INFO_MEM_MAP) {
    INFO2("* Multiboot provided memory map  (%i entries)",bootinfo->mmap_length / sizeof(multiboot_memory_map_t));
    gsl::span<multiboot_memory_map_t> mmap { reinterpret_cast<multiboot_memory_map_t*>(bootinfo->mmap_addr),
        (int)(bootinfo->mmap_length / sizeof(multiboot_memory_map_t))};

    for (auto map : mmap) {
      const char* str_type = map.type & MULTIBOOT_MEMORY_AVAILABLE ? "FREE" : "RESERVED";
      INFO2("\t 0x%08llx - 0x%08llx %s (%llu Kb.)",
            map.addr, map.addr + map.len - 1, str_type, map.len / 1024 );
      /*if (map.addr + map.len > mem_high_end)
        break;*/
    }
    printf("\n");
  }
}

/// SERVICE RELATED ///

// the name of the current service (built from another module)
extern "C" {
  __attribute__((weak))
  const char* service_name__ = "(missing service name)";
}

std::string Service::name() {
  return service_name__;
}

const std::string& Service::command_line()
{
  return os_cmdline;
}

// functions that we can override if we want to
__attribute__((weak))
void Service::ready() {}

__attribute__((weak))
void Service::stop() {}
