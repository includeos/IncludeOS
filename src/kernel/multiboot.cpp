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
#include <kprint>
#include <boot/multiboot.h>
#include <kernel/memory.hpp>

//#define DEBUG
#if defined(DEBUG)
#define debug(X,...)  kprintf(X,##__VA_ARGS__);
#else
#define debug(X,...)
#endif

#define MYINFO(X,...) INFO("Kernel", X, ##__VA_ARGS__)

extern uintptr_t _end;
extern "C" uintptr_t _multiboot_free_begin(uintptr_t);

using namespace util::bitops;
using namespace util::literals;

multiboot_info_t* OS::bootinfo()
{
  // NOTE: the address is 32-bit and not a pointer
  extern uint32_t __multiboot_addr;
  return (multiboot_info_t*) (uintptr_t) __multiboot_addr;
}

// Deterimine the end of multiboot provided data
// (e.g. multiboot's data area as offset to the _end symbol)
uintptr_t _multiboot_free_begin(uintptr_t boot_addr)
{
  multiboot_info_t* bootinfo = (multiboot_info_t*) boot_addr;
  uintptr_t multi_end = reinterpret_cast<uintptr_t>(&_end);

  debug("* Multiboot begin: 0x%x \n", bootinfo);
  if (bootinfo->flags & MULTIBOOT_INFO_CMDLINE
      and bootinfo->cmdline > multi_end)
  {
    debug("* Multiboot cmdline @ 0x%x: %s \n", bootinfo->cmdline, (char*)bootinfo->cmdline);
    // We can't use a cmdline that's either insde our ELF or pre-ELF area
    Expects(bootinfo->cmdline > multi_end
            or bootinfo->cmdline < 0x100000);

    if (bootinfo->cmdline > multi_end) {
      auto* cmdline_ptr = (const char*) (uintptr_t) bootinfo->cmdline;
      // Set free begin to after the cmdline string
      multi_end = bootinfo->cmdline + strlen(cmdline_ptr) + 1;
    }
  }

  debug("* Multiboot end: 0x%x \n", multi_end);
  if (bootinfo->mods_count == 0)
      return multi_end;

  multiboot_module_t* mods_list =  (multiboot_module_t*)bootinfo->mods_addr;
  debug("* Module list @ %p \n",mods_list);

  for (multiboot_module_t* mod = mods_list;
       mod < mods_list + bootinfo->mods_count;
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

void OS::multiboot(uint32_t boot_addr)
{
  MYINFO("Booted with multiboot");
  auto* bootinfo_ = bootinfo();
  INFO2("* Boot flags: %#x", bootinfo_->flags);

  if (bootinfo_->flags & MULTIBOOT_INFO_MEMORY) {
    uint32_t mem_low_start = 0;
    uint32_t mem_low_end = (bootinfo_->mem_lower * 1024) - 1;
    uint32_t mem_low_kb = bootinfo_->mem_lower;
    uint32_t mem_high_start = 0x100000;
    uint32_t mem_high_end = mem_high_start + (bootinfo_->mem_upper * 1024) - 1;
    uint32_t mem_high_kb = bootinfo_->mem_upper;

    OS::memory_end_ = mem_high_end;

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

  if (bootinfo_->flags & MULTIBOOT_INFO_CMDLINE) {
    const auto* cmdline = (const char*) (uintptr_t) bootinfo_->cmdline;
    INFO2("* Booted with parameters @ %p: %s", cmdline, cmdline);
    OS::cmdline = strdup(cmdline);
  }

  if (bootinfo_->flags & MULTIBOOT_INFO_MEM_MAP) {
    INFO2("* Multiboot provided memory map  (%i entries @ %p)",
          bootinfo_->mmap_length / sizeof(multiboot_memory_map_t), (void*)bootinfo_->mmap_addr);
    gsl::span<multiboot_memory_map_t> mmap {
        reinterpret_cast<multiboot_memory_map_t*>(bootinfo_->mmap_addr),
        (int)(bootinfo_->mmap_length / sizeof(multiboot_memory_map_t))
      };

    for (auto map : mmap)
    {
      const char* str_type = map.type & MULTIBOOT_MEMORY_AVAILABLE ? "FREE" : "RESERVED";
      const uintptr_t addr = map.addr;
      const uintptr_t size = map.len;
      INFO2("  0x%010llx - 0x%010llx %s (%llu Kb.)",
            addr, addr + size - 1, str_type, size / 1024 );

      if (not (map.type & MULTIBOOT_MEMORY_AVAILABLE)) {

        if (util::bits::is_aligned<4_KiB>(map.addr)) {
          os::mem::map({addr, addr, {os::mem::Access::read | os::mem::Access::write}, size},
                       "Reserved (Multiboot)");
          continue;
        }

        // For non-aligned addresses, assign
        memory_map().assign_range({addr, addr + size - 1, "Reserved (Multiboot)"});
      }
      else
      {
        // Map as free memory
        //os::mem::map_avail({map.addr, map.addr, {os::mem::Access::read | os::mem::Access::write}, map.len}, "Reserved (Multiboot)");
      }
    }
    printf("\n");
  }

  Span_mods mods = modules();

  if (not mods.empty()) {
    MYINFO("OS loaded with %i modules", mods.size());
    for (auto mod : mods) {
      INFO2("* %s @ 0x%x - 0x%x, size: %ib",
            reinterpret_cast<char*>(mod.cmdline),
            mod.mod_start, mod.mod_end, mod.mod_end - mod.mod_start);
    }
  }
}
