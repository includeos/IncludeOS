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

#include <common>
#include <arch.hpp>
#include <kernel/memmap.hpp>
#include <kernel/rtc.hpp>
#include <hertz>
#include <string>
#include <vector>
#include <boot/multiboot.h>

/**
 *  The entrypoint for OS services
 *
 *  @note For device access, see Dev
 */
class OS {
public:
  using print_func  = delegate<void(const char*, size_t)>;
  using Plugin      = delegate<void()>;
  using Span_mods   = gsl::span<multiboot_module_t>;

  /**
   * Returns the OS version string
   **/
  static const std::string& version() noexcept
  { return version_str_; }

  /**
   * Returns the CPU architecture for which the OS was built
   **/
  static const std::string& arch() noexcept
  { return arch_str_; }


  /**
   *  Returns the commandline arguments provided,
   *  if any, to the VM passed on by multiboot or
   *  other mechanisms. The first argument is always
   *  the binary name.
  **/
  static const char* cmdline_args() noexcept;

  /** Clock cycles since boot. */
  static uint64_t cycles_since_boot() {
    return __arch_cpu_cycles();
  }
  /** micro seconds since boot */
  static int64_t micros_since_boot() noexcept;

  /** Timestamp for when OS was booted */
  static RTC::timestamp_t boot_timestamp();

  /** Uptime in whole seconds. */
  static RTC::timestamp_t uptime();

  /** Time spent sleeping (halt) in cycles */
  static uint64_t cycles_asleep() noexcept;

  /** Time spent sleeping (halt) in micros */
  static uint64_t micros_asleep() noexcept;


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
  static bool is_running() noexcept {
    return power_;
  }

  /**
   *  Returns true when the OS has passed the boot sequence, and
   *  is at least processing plugins and about to call Service::start
   */
  static bool is_booted() noexcept {
    return boot_sequence_passed_;
  }

  static bool block_drivers_ready() noexcept {
    return m_block_drivers_ready;
  }

  /**
   *  Returns true when the OS is currently panicking
   */
  static bool is_panicking() noexcept;

  /**
   * Sometimes the OS just has a bad day and crashes
   * The on_panic handler will be called directly after a panic,
   * or any condition which will deliberately cause the OS to become
   * unresponsive. After the handler is called, the OS goes to sleep.
   * This handler can thus be used to, for example, automatically
   * have the OS restart on any crash.
  **/
  typedef void (*on_panic_func) (const char*);
  static void on_panic(on_panic_func);

  /**
   *  Write data to standard out callbacks
   */
  static void print(const char* ptr, const size_t len);

  /**
   *  Add handler for standard output.
   */
  static void add_stdout(print_func func);

  /**
   *  The default output method preferred by each platform
   *  Directly writes the string to its output mechanism
   **/
  static void default_stdout(const char*, size_t);

  /** Memory page helpers */
  static constexpr uint32_t page_size() noexcept {
    return 4096;
  }
  static constexpr uint32_t addr_to_page(uintptr_t addr) noexcept {
    return addr >> PAGE_SHIFT;
  }
  static constexpr uintptr_t page_to_addr(uint32_t page) noexcept {
    return page << PAGE_SHIFT;
  }

  /** Total used dynamic memory, in bytes */
  static uintptr_t heap_usage() noexcept;

  /** Attempt to trim the heap end, reducing the size */
  static void heap_trim() noexcept;

  /** First address of the heap **/
  static uintptr_t heap_begin() noexcept;

  /** Last used address of the heap **/
  static uintptr_t heap_end() noexcept;

  /** Resize the heap if possible. Return (potentially) new size. **/
  static uintptr_t resize_heap(size_t size);

  /** The maximum last address of the dynamic memory area (heap) */
  static uintptr_t heap_max() noexcept;

  /** The end of usable memory **/
  static uintptr_t memory_end() noexcept {
    return memory_end_;
  }

  /**
   *  Returns true when the current OS comes from a live update,
   *  as opposed to booting from either a rollback or a normal boot
   */
  static bool is_live_updated() noexcept;

  /** Returns the automatic location set aside for storing system and program state **/
  static void* liveupdate_storage_area() noexcept;

  /**
   * A map of memory ranges. The key is the starting address in numeric form.
   * @note : the idea is to avoid raw pointers whenever possible
  **/
  static Memory_map& memory_map() {
    static  Memory_map memmap {};
    return memmap;
  }

  /** Get "kernel modules", provided by multiboot */
  static Span_mods modules();

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

  /** Initialize platform, devices etc. */
  static void start(uint32_t boot_magic, uint32_t boot_addr);

  static void start(char *cmdline, uintptr_t mem_size);

  /** Initialize common subsystems, call Service::start */
  static void post_start();

private:
  /** Process multiboot info. Called by 'start' if multibooted **/
  static void multiboot(uint32_t boot_addr);

  static multiboot_info_t* bootinfo();

  /** Boot with no multiboot params */
  static void legacy_boot();

  /** Resume stuff from a soft reset **/
  static bool is_softreset_magic(uint32_t value);
  static void resume_softreset(intptr_t boot_addr);

  static constexpr int PAGE_SHIFT = 12;
  static bool power_;
  static bool boot_sequence_passed_;
  static bool m_is_live_updated;
  static bool m_block_drivers_ready;
  static MHz cpu_mhz_;

  static RTC::timestamp_t booted_at_;
  static uintptr_t liveupdate_loc_;
  static std::string version_str_;
  static std::string arch_str_;
  static uintptr_t memory_end_;
  static uintptr_t heap_max_;
  static const uintptr_t elf_binary_size_;
  static const char* cmdline;

  // Prohibit copy and move operations
  OS(OS&)  = delete;
  OS(OS&&) = delete;
  OS& operator=(OS&)  = delete;
  OS& operator=(OS&&)  = delete;
  ~OS() = delete;
  // Prohibit construction
  OS() = delete;

  friend void __platform_init();
}; //< OS

inline bool OS::is_live_updated() noexcept {
  return OS::m_is_live_updated;
}

inline OS::Span_mods OS::modules()
{
  auto* bootinfo_ = bootinfo();
  if (bootinfo_ and bootinfo_->flags & MULTIBOOT_INFO_MODS and bootinfo_->mods_count) {

    Expects(bootinfo_->mods_count < std::numeric_limits<int>::max());

    return Span_mods{
      reinterpret_cast<multiboot_module_t*>(bootinfo_->mods_addr),
        static_cast<int>(bootinfo_->mods_count) };
  }
  return nullptr;
}

#endif //< KERNEL_OS_HPP
