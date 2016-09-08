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

#include <string>
#include <sstream>
#include <common>
#include <kernel/memmap.hpp>
#include <hw/cpu.hpp>
#include <hertz>
#include <vector>
#include <kernel/rtc.hpp>

namespace hw{ class Serial; }

/**
 *  The entrypoint for OS services
 *
 *  @note For device access, see Dev
 */
class OS {
public:
  using rsprint_func = delegate<void(const char*, size_t)>;
  using Custom_init = delegate<void()>;

  /* Get the version of the os */
  static std::string version()
  { return version_field; }

  /** Clock cycles since boot. */
  static uint64_t cycles_since_boot() {
    return hw::CPU::rdtsc();
  }
  /** micro seconds since boot */
  static int64_t micros_since_boot() {
    return cycles_since_boot() / cpu_mhz_.count();
  }

  /** Timestamp for when OS was booted */
  static RTC::timestamp_t boot_timestamp()
  { return booted_at_; }

  /** Uptime in whole seconds. */
  static RTC::timestamp_t uptime() {
    return RTC::now() - booted_at_;
  }

  static MHz cpu_freq()
  { return cpu_mhz_; }

  /**
   * Shutdown operating system
   *
   **/
  static void shutdown();

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
  static void start(uint32_t boot_magic, uint32_t boot_addr);

  /**
   *  Halt until next interrupt.
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

  /** The maximum last address of the dynamic memory area (heap) */
  static uintptr_t heap_max();

  /**
   * A map of memory ranges. The key is the starting address in numeric form.
   * @note : the idea is to avoid raw pointers whenever possible
   */
  static Memory_map& memory_map () noexcept {
    static  Memory_map memmap_ {};
    return memmap_;
  };

  /**
   * Register a custom initialization function. The provided delegate is
   * guaranteed to be called after global constructors and device initialization
   * and before Service::start, provided that this funciton was called by a
   * global constructor.
   * @param delg : A delegate to be called
   * @param name : A human readable identifier
   **/
  static void register_custom_init(Custom_init delg, const char* name);

private:

  /** Process multiboot info. Called by 'start' if multibooted **/
  static void multiboot(uint32_t boot_magic, uint32_t boot_addr);

  static const int page_shift_ = 12;

  /** Indicate if the OS is running. */
  static bool power_;

  /** The main event loop. Check interrupts, timers etc., and do callbacks. */
  static void event_loop();

  static MHz cpu_mhz_;

  static rsprint_func rsprint_handler_;

  static hw::Serial& com1;

  static RTC::timestamp_t booted_at_;
  static std::string version_field;

  struct Custom_init_struct {
    Custom_init_struct(Custom_init f, const char* n)
      : func_{f}, name_{n}
    {}

    Custom_init func_;
    const char* name_;
  };

  static std::vector<Custom_init_struct> custom_init_;

  static uintptr_t low_memory_size_;
  static uintptr_t high_memory_size_;
  static uintptr_t heap_max_;
  static const uintptr_t elf_binary_size_;

  // Prohibit copy and move operations
  OS(OS&)  = delete;
  OS(OS&&) = delete;
  OS& operator=(OS&)  = delete;
  OS& operator=(OS&&)  = delete;
  ~OS() = delete;
  // Prohibit construction
  OS() = delete;
  friend void begin_stack_sampling(uint16_t);

}; //< OS

#endif //< KERNEL_OS_HPP
