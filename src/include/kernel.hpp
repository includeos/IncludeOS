// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2018 IncludeOS AS, Oslo, Norway
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
    uintptr_t heap_begin       = 0;
    uintptr_t heap_max         = default_max_mem;;
    uintptr_t memory_end       = default_max_mem;;
    const char* cmdline        = nullptr;
    int  panics                = 0;
    os::Panic_action panic_action {};
    util::KHz cpu_khz {-1};
    //const uintptr_t elf_binary_size = 0;
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

  inline const char* cmdline() {
    return state().cmdline;
  }

  inline bool is_panicking() noexcept {
    return state().panics > 0;
  }

  inline int panics() {
    return state().panics;
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

  multiboot_info_t* bootinfo();

  /** Boot with no multiboot params */
  void legacy_boot();

  //static constexpr int PAGE_SHIFT = 12;
  //static


  void default_stdout(const char*, size_t);

  /** Resume stuff from a soft reset **/
  bool is_softreset_magic(uint32_t value);
  uintptr_t softreset_memory_end(intptr_t boot_addr);
  void resume_softreset(intptr_t boot_addr);

  /** Returns the amount of memory set aside for LiveUpdate */
  size_t liveupdate_phys_size(size_t) noexcept;

  /** Computes the physical location of LiveUpdate storage area */
  uintptr_t liveupdate_phys_loc(size_t) noexcept;

  inline void* liveupdate_storage_area() noexcept {
    return (void*)state().liveupdate_loc;
  }

  void setup_liveupdate(uintptr_t phys = 0);

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
