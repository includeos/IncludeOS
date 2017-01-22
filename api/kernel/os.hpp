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

/**
 *  The entrypoint for OS services
 *
 *  @note For device access, see Dev
 */
class OS {
public:
  using print_func  = delegate<void(const char*, size_t)>;
  using Plugin = delegate<void()>;

  /**
   * Returns the version of the OS from when 
   * the service was built.
  **/
  static const std::string& version() noexcept
  { return version_field; }

  /** 
   *  Returns the commandline arguments provided,
   *  if any, to the VM passed on by multiboot or
   *  other mechanisms. The first argument is always
   *  the binary name.
  **/
  static const std::string& cmdline_args() noexcept;

  /** Clock cycles since boot. */
  static uint64_t cycles_since_boot() {
    return hw::CPU::rdtsc();
  }
  /** micro seconds since boot */
  static int64_t micros_since_boot() {
    return cycles_since_boot() / cpu_freq().count();
  }

  /** Timestamp for when OS was booted */
  static RTC::timestamp_t boot_timestamp()
  { return booted_at_; }

  /** Uptime in whole seconds. */
  static RTC::timestamp_t uptime() {
    return RTC::now() - booted_at_;
  }

  static MHz cpu_freq() noexcept
  { return cpu_mhz_; }

  /**
   * Reboot operating system
   *
   **/
  static void reboot();

  /**
   * Shutdown operating system
   *
   **/
  static void shutdown();

  /**
   *  Halt until next interrupt.
   *
   *  @Warning If there is no regular timer interrupt (i.e. from PIT / APIC)
   *  we'll stay asleep.
   */
  static void halt();

  /**
   *  Returns true when the OS will still be running, and not shutting down.
   */
  static bool is_running() {
    return power_;
  }

  /**
   * Sometimes the OS just has a bad day and crashes
   * The on_panic handler will be called directly after a panic,
   * or any condition which will deliberately cause the OS to become
   * unresponsive. After the handler is called, the OS goes to sleep.
   * This handler can thus be used to, for example, automatically 
   * have the OS restart on any crash.
  **/
  typedef void (*on_panic_func) ();
  static void on_panic(on_panic_func);

  /**
   *  Write data to standard out callbacks
   */
  static size_t print(const char* ptr, const size_t len);

  /**
   *  Add handler for standard output.
   */
  static void add_stdout(print_func func);
  /**
   *  Add "default" serial port output
  **/
  static void add_stdout_default_serial();

  /** Memory page helpers */
  static constexpr uint32_t page_size() noexcept {
    return 4096;
  }
  static constexpr uint32_t page_nr_from_addr(uint32_t x) noexcept {
    return x >> PAGE_SHIFT;
  }
  static constexpr uint32_t base_from_page_nr(uint32_t x) noexcept {
    return x << PAGE_SHIFT;
  }

  /** First address of the heap **/
  static uintptr_t heap_begin() noexcept;

  /** Last used address of the heap **/
  static uintptr_t heap_end() noexcept;

  /** The maximum last address of the dynamic memory area (heap) */
  static uintptr_t heap_max() noexcept{
    return heap_max_;
  };

  /** Currently used dynamic memory, in bytes */
  static uintptr_t heap_usage() noexcept {
    return heap_end() - heap_begin();
  };

  /** Resize the heap if possible. Return (potentially) new size. **/
  static uintptr_t resize_heap(size_t size);

  /** The end of usable memory **/
  static inline uintptr_t memory_end(){
    return memory_end_;
  }

  /** time spent sleeping (halt) in cycles */
  static uint64_t get_cycles_halt() noexcept;

  /** total time spent in cycles */
  static uint64_t get_cycles_total() noexcept;

  /**
   * A map of memory ranges. The key is the starting address in numeric form.
   * @note : the idea is to avoid raw pointers whenever possible
  **/
  static Memory_map& memory_map() {
    static  Memory_map memmap {};
    return memmap;
  }

  /**
   * Register a custom initialization function. The provided delegate is
   * guaranteed to be called after global constructors and device initialization
   * and before Service::start, provided that this funciton was called by a
   * global constructor.
   * @param delg : A delegate to be called
   * @param name : A human readable identifier
  **/
  static void register_plugin(Plugin delg, const char* name);

  /**
   * Block for a while, e.g. until the next round in the event loop
   **/
  static void block();


  /** The main event loop. Check interrupts, timers etc., and do callbacks. */
  static void event_loop();

  /** Start the OS.  @todo Should be `init()` - and not accessible from ABI */
  static void start(uint32_t boot_magic, uint32_t boot_addr);

private:

  /** Process multiboot info. Called by 'start' if multibooted **/
  static void multiboot(uint32_t boot_magic, uint32_t boot_addr);

  /** Boot with no multiboot params */
  static void legacy_boot();

  /** Resume stuff from a soft reset **/
  static bool is_softreset_magic(uint32_t value);
  static void resume_softreset(intptr_t boot_addr);

  static constexpr int PAGE_SHIFT = 12;

  /** Indicate if the OS is running. */
  static bool power_;

  static MHz cpu_mhz_;

  static RTC::timestamp_t booted_at_;
  static std::string version_field;

  struct Plugin_struct {
    Plugin_struct(Plugin f, const char* n)
      : func_{f}, name_{n}
    {}

    Plugin func_;
    const char* name_;
  };

  static std::vector<Plugin_struct> plugins_;

  static uintptr_t low_memory_size_;
  static uintptr_t high_memory_size_;
  static uintptr_t memory_end_;
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

}; //< OS

#endif //< KERNEL_OS_HPP
