// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017 Oslo and Akershus University College of Applied Sciences
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


#include <os>
#include <kernel.hpp>
#include <kprint>
#include <boot/multiboot.h>
#include <kernel/memory.hpp>

#define DEBUG_MULTIBOOT
#if defined(DEBUG_MULTIBOOT)
#undef debug
#define debug(X,...)  kprintf(X,##__VA_ARGS__);
#define MYINFO(X,...) kprintf("<Multiboot>" X "\n", ##__VA_ARGS__)
#undef INFO2
#define INFO2(X,...) kprintf("\t" X "\n", ##__VA_ARGS__)
#else
#define debug(X,...)
#define MYINFO(X,...) INFO("Kernel", X, ##__VA_ARGS__)
#endif


extern uintptr_t _end;


using namespace util::bitops;
using namespace util::literals;

static inline multiboot_info_t* bootinfo(uint32_t addr)
{
  // NOTE: the address is 32-bit and not a pointer
  return (multiboot_info_t*) (uintptr_t) addr;
}
#if defined(ARCH_aarch64)
  uint32_t dummy[24];
  uintptr_t __multiboot_addr=(uintptr_t)&dummy[0];
#else
  extern uint32_t __multiboot_addr;
#endif

multiboot_info_t* kernel::bootinfo()
{
  return (multiboot_info_t*) (uintptr_t) __multiboot_addr;
}

uintptr_t _multiboot_memory_end(uintptr_t boot_addr) {
  auto* info = bootinfo(boot_addr);
  if (info->flags & MULTIBOOT_INFO_MEMORY) {
    return 0x100000 + (info->mem_upper * 1024);
  }
  return os::Arch::max_canonical_addr;
}

// Deterimine the end of multiboot provided data
// (e.g. multiboot's data area as offset to the _end symbol)
uintptr_t _multiboot_free_begin(uintptr_t boot_addr)
{
  auto* info = bootinfo(boot_addr);
  uintptr_t multi_end = reinterpret_cast<uintptr_t>(&_end);

  debug("* Multiboot begin: 0x%x \n", info);
  if (info->flags & MULTIBOOT_INFO_CMDLINE
      and info->cmdline > multi_end)
  {
    debug("* Multiboot cmdline @ 0x%x: %s \n", info->cmdline, (char*)info->cmdline);
    // We can't use a cmdline that's either insde our ELF or pre-ELF area
    Expects(info->cmdline > multi_end
            or info->cmdline < 0x100000);

    if (info->cmdline > multi_end) {
      auto* cmdline_ptr = (const char*) (uintptr_t) info->cmdline;
      // Set free begin to after the cmdline string
      multi_end = info->cmdline + strlen(cmdline_ptr) + 1;
    }
  }

  debug("* Multiboot end: 0x%x \n", multi_end);
  if (info->mods_count == 0)
      return multi_end;

  auto* mods_list = (multiboot_module_t*) (uintptr_t) info->mods_addr;
  debug("* Module list @ %p \n",mods_list);

  for (multiboot_module_t* mod = mods_list;
       mod < mods_list + info->mods_count;
       mod ++) {

    debug("\t * Module @ %#x \n", mod->mod_start);
    debug("\t * Args: %s \n ", (char*) (uintptr_t) mod->cmdline);
    debug("\t * End: %#x \n ", mod->mod_end);

    if (mod->mod_end > multi_end)
      multi_end = mod->mod_end;

  }

  debug("* Multiboot end: 0x%x \n", multi_end);
  return multi_end;
}

void kernel::multiboot(uint32_t boot_addr)
{
  MYINFO("Booted with multiboot");
  auto* info = ::bootinfo(boot_addr);
  INFO2("* Boot flags: %#x", info->flags);

  if (info->flags & MULTIBOOT_INFO_MEMORY) {
    uint32_t mem_low_start = 0;
    uint32_t mem_low_end = (info->mem_lower * 1024) - 1;
    uint32_t mem_low_kb = info->mem_lower;
    uint32_t mem_high_start = 0x100000;
    uint32_t mem_high_end = mem_high_start + (info->mem_upper * 1024) - 1;
    uint32_t mem_high_kb = info->mem_upper;

    INFO2("* Valid memory (%i Kib):", mem_low_kb + mem_high_kb);
    INFO2("  0x%08x - 0x%08x (%i Kib)",
          mem_low_start, mem_low_end, mem_low_kb);
    INFO2("  0x%08x - 0x%08x (%i Kib)",
          mem_high_start, mem_high_end, mem_high_kb);
    INFO2("");
  }
  else {
    INFO2("No memory information from multiboot");
  }

  if (info->flags & MULTIBOOT_INFO_CMDLINE) {
    const auto* cmdline = (const char*) (uintptr_t) info->cmdline;
    INFO2("* Booted with parameters @ %p: %s", cmdline, cmdline);
    kernel::state().cmdline = strdup(cmdline);
  }

  if (info->flags & MULTIBOOT_INFO_MEM_MAP) {
    INFO2("* Multiboot provided memory map  (%zu entries @ %p)",
          info->mmap_length / sizeof(multiboot_memory_map_t),
          (void*) (uintptr_t) info->mmap_addr);
    gsl::span<multiboot_memory_map_t> mmap {
        reinterpret_cast<multiboot_memory_map_t*>(info->mmap_addr),
        (int)(info->mmap_length / sizeof(multiboot_memory_map_t))
      };

    for (auto map : mmap)
    {
      const char* str_type = map.type & MULTIBOOT_MEMORY_AVAILABLE ? "FREE" : "RESERVED";
      const uintptr_t addr = map.addr;
      const uintptr_t size = map.len;
      INFO2("  0x%010zx - 0x%010zx %s (%zu Kb.)",
            addr, addr + size - 1, str_type, size / 1024 );

      if (not (map.type & MULTIBOOT_MEMORY_AVAILABLE)) {

        if (util::bits::is_aligned<4_KiB>(map.addr)) {
          os::mem::map({addr, addr, os::mem::Access::read | os::mem::Access::write, size},
                       "Reserved (Multiboot)");
          continue;
        }

        // For non-aligned addresses, assign
        os::mem::vmmap().assign_range({addr, addr + size - 1, "Reserved (Multiboot)"});
      }
      else
      {
        // Map as free memory
        //os::mem::map_avail({map.addr, map.addr, {os::mem::Access::read | os::mem::Access::write}, map.len}, "Reserved (Multiboot)");
      }
    }
    printf("\n");
  }

  auto mods = os::modules();

  if (not mods.empty()) {
    MYINFO("OS loaded with %zu modules", mods.size());
    for (auto mod : mods) {
      INFO2("* %s @ 0x%x - 0x%x, size: %ib",
            reinterpret_cast<char*>(mod.params),
            mod.mod_start, mod.mod_end, mod.mod_end - mod.mod_start);
    }
  }
}
