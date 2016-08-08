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

#ifndef KERNEL_OS_HPP
#define KERNEL_OS_HPP

#ifndef OS_VERSION
#define OS_VERSION "v?.?.?"
#endif

#include <string>
#include <common>
#include <hw/cpu.hpp>
#include <hw/pit.hpp>

namespace hw{ class Serial; }

/**
 *  The entrypoint for OS services
 *
 *  @note For device access, see Dev
 */
class OS {
public:
  using rsprint_func = delegate<void(const char*, size_t)>;

  /* Get the version of the os */
  static std::string version()
  { return std::string(OS_VERSION); }

  /** Clock cycles since boot. */
  static uint64_t cycles_since_boot() {
    return hw::CPU::rdtsc();
  }
  
  /** Uptime in seconds. */
  static double uptime() {
    return cycles_since_boot() / Hz(cpu_mhz_).count();
  }

  /**
   *  Write a cstring to serial port. @todo Should be moved to Dev::serial(n).
   *
   *  @param ptr: The string to write to serial port
   */
  static size_t rsprint(const char* ptr);
  static size_t rsprint(const char* ptr, const size_t len);

  /**
   *  Write a character to serial port.
   *
   *  @param c: The character to print to serial port
   */
  static void rswrite(const char c);

  /**
   *  Write to serial port with rswrite.
   */
  static void default_rsprint(const char*, size_t);

  /** Start the OS.  @todo Should be `init()` - and not accessible from ABI */
  static void start();

  /**
   *  Halt until next inerrupt.
   *
   *  @Warning If there is no regular timer interrupt (i.e. from PIT / APIC)
   *  we'll stay asleep.
   */
  static void halt();

  /**
   *  Set handler for serial output.
   */
  static void set_rsprint(rsprint_func func) {
    rsprint_handler_ = func;
  }

  /** Memory page helpers */
  static inline constexpr uint32_t page_size() {
    return 4096;
  }
  static inline constexpr uint32_t page_nr_from_addr(uint32_t x){
    return x >> page_shift_;
  }
  static inline constexpr uint32_t base_from_page_nr(uint32_t x){
    return x << page_shift_;
  }

  /** Currently used dynamic memory, in bytes */
  static uintptr_t heap_usage();

private:
  static const int page_shift_ = 12;

  /** Indicate if the OS is running. */
  static bool power_;

  /** The main event loop. Check interrupts, timers etc., and do callbacks. */
  static void event_loop();

  static MHz cpu_mhz_;

  static rsprint_func rsprint_handler_;

  static hw::Serial& com1;

  // Prohibit copy and move operations
  OS(OS&)  = delete;
  OS(OS&&) = delete;

  // Prohibit construction
  OS() = delete;
  friend void begin_stack_sampling(uint16_t);
}; //< OS

#endif //< KERNEL_OS_HPP
