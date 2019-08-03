
#ifndef KERNEL_HPP
#define KERNEL_HPP

#include <hal/machine.hpp>
#include <util/units.hpp>
#include <boot/multiboot.h>

namespace kernel {

  using namespace util;
  constexpr size_t default_max_mem = 2_GiB;
  constexpr uintptr_t page_shift   = 12;

  struct State {
    bool running               = true;
    bool boot_sequence_passed  = false;
    bool libc_initialized      = false;
    bool block_drivers_ready   = false;
    bool timestamps            = false;
    bool timestamps_ready      = false;
    bool is_live_updated       = false;
    uintptr_t liveupdate_loc   = 0;
    uintptr_t liveupdate_phys  = 0;
    uintptr_t liveupdate_size  = 0;
    uintptr_t heap_begin       = 0;
    uintptr_t heap_max         = default_max_mem;;
    uintptr_t memory_end       = default_max_mem;;
    const char* cmdline        = nullptr;
    int  panics                = 0;
    os::Panic_action panic_action {};
    util::KHz cpu_khz {-1};
	// Memory Mapping buffer (stored for live updates)
	void*    mmap_addr  = nullptr;
	uint32_t mmap_size  = 0;
  };

  State& state() noexcept;

  inline bool is_running() noexcept {
    return state().running;
  }

  inline bool is_booted() noexcept {
    return state().boot_sequence_passed;
  }

  inline bool libc_initialized() noexcept {
    return state().libc_initialized;
  }

  inline bool block_drivers_ready() noexcept {
    return state().block_drivers_ready;
  }

  inline bool timestamps() noexcept {
    return state().timestamps;
  }

  inline bool timestamps_ready() noexcept {
    return state().timestamps_ready;
  }

  inline bool is_live_updated() noexcept {
    return state().is_live_updated;
  }

  inline const char* cmdline() noexcept {
    return state().cmdline;
  }

  inline int panics() noexcept {
    return state().panics;
  }

  inline bool is_panicking() noexcept {
    return panics() > 0;
  }

  inline os::Panic_action panic_action() noexcept {
    return state().panic_action;
  }

  inline void set_panic_action(os::Panic_action action) noexcept {
    state().panic_action = action;
  }

  using ctor_t = void (*)();
  inline void run_ctors(ctor_t* begin, ctor_t* end)
  {
  	for (; begin < end; begin++) (*begin)();
  }

  inline util::KHz cpu_freq() {
    return state().cpu_khz;
  }

  /** First address of the heap **/
  inline uintptr_t heap_begin() noexcept {
    return state().heap_begin;
  }

  /** The maximum last address of the dynamic memory area (heap) */
  inline uintptr_t heap_max() noexcept {
    return state().heap_max;
  }

  /** Initialize platform, devices etc. */
  void start(uint32_t boot_magic, uint32_t boot_addr);
  void start(uint64_t fdt);
  void start(const char* cmdline);

  /** Initialize common subsystems, call Service::start */
  void post_start();

  /** Process multiboot info. Called by 'start' if multibooted **/
  void multiboot(uint32_t boot_addr);
  void multiboot_mmap(void* addr, size_t);

  multiboot_info_t* bootinfo();

  /** Boot with no multiboot params */
  void legacy_boot();

  void default_stdout(const char*, size_t);

  /** Resume stuff from a soft reset **/
  bool is_softreset_magic(uint32_t value);
  uintptr_t softreset_memory_end(intptr_t boot_addr);
  void resume_softreset(intptr_t boot_addr);

  inline void* liveupdate_storage_area() noexcept {
    return (void*) state().liveupdate_loc;
  }
  inline size_t liveupdate_storage_size() noexcept {
    return state().liveupdate_size;
  }

  void setup_liveupdate();

  bool heap_ready();

  void init_heap(os::Machine::Memory& mem);

  /** The end of usable memory **/
  inline uintptr_t memory_end() noexcept {
    return state().memory_end;
  }

  /** Total used dynamic memory, in bytes */
  size_t heap_usage() noexcept;

  /** Total free heap, as far as the OS knows, in bytes */
  size_t heap_avail() noexcept;


  void init_heap(uintptr_t phys_begin, size_t size) noexcept;

  /** Last used address of the heap **/
  uintptr_t heap_end() noexcept;


  void default_exit() __attribute__((noreturn));

  constexpr uint32_t page_size() noexcept {
    return 4096;
  }

  constexpr uint32_t addr_to_page(uintptr_t addr) noexcept {
    return addr >> page_shift;
  }

  constexpr uintptr_t page_to_addr(uint32_t page) noexcept {
    return page << page_shift;
  }
}

#endif
