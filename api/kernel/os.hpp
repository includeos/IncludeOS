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
#include <util/units.hpp>
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


  /**
   *  The default output method preferred by each platform
   *  Directly writes the string to its output mechanism
   **/
  //static void default_stdout(const char*, size_t);

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

  //static void init_heap(uintptr_t phys_begin, size_t size) noexcept;

  /**
   * A map of memory ranges. The key is the starting address in numeric form.
   * @note : the idea is to avoid raw pointers whenever possible
  **/
  static Memory_map& memory_map() {
    static  Memory_map memmap {};
    return memmap;
  }

  using Span_mods = gsl::span<multiboot_module_t>;

  /** Get "kernel modules", provided by multiboot */
  static Span_mods modules();

  using Plugin = delegate<void()>;
  /**
   * Register a custom initialization function. The provided delegate is
   * guaranteed to be called after global constructors and device initialization
   * and before Service::start, provided that this funciton was called by a
   * global constructor.
   * @param delg : A delegate to be called
   * @param name : A human readable identifier
  **/
  static void register_plugin(Plugin delg, const char* name);


  /** Initialize platform, devices etc. */
  static void start(uint32_t boot_magic, uint32_t boot_addr);

  static void start(const char* cmdline);

  /** Initialize common subsystems, call Service::start */
  static void post_start();



private:
  /** Process multiboot info. Called by 'start' if multibooted **/
  static void multiboot(uint32_t boot_addr);

  static multiboot_info_t* bootinfo();

  /** Boot with no multiboot params */
  static void legacy_boot();

  static constexpr int PAGE_SHIFT = 12;
  static const uintptr_t elf_binary_size_;

  friend void __platform_init();
}; //< OS


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
