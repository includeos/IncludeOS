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

#define MYINFO(X,...) INFO("Kernel", X, ##__VA_ARGS__)

extern "C" {

  extern uintptr_t _end;

  // Deterimine the end of multiboot provided data
  // (e.g. multiboot's data area as offset to the _end symbol)
  uintptr_t _multiboot_free_begin(uintptr_t boot_addr){

    multiboot_info_t* bootinfo = (multiboot_info_t*) boot_addr;
    uintptr_t multi_end = reinterpret_cast<uintptr_t>(&_end);

    if (bootinfo->flags & MULTIBOOT_INFO_CMDLINE
        and bootinfo->cmdline > multi_end) {

      debug("* Multiboot cmdline @ 0x%x: %s \n", bootinfo->cmdline, (char*)bootinfo->cmdline);

      // Set free begin to after the cmdline string
      multi_end = bootinfo->cmdline +
        strlen(reinterpret_cast<const char*>(bootinfo->cmdline)) + 1;
    }

    debug("* Multiboot end: 0x%x \n", multi_end);


    if (not bootinfo->mods_count)
      return multi_end;

    multiboot_module_t* mods_list =  (multiboot_module_t*)bootinfo->mods_addr;
    debug("* Module list @ %p \n",mods_list);

    for (multiboot_module_t* mod = mods_list;
         mod < mods_list + bootinfo->mods_count;
         mod ++) {

      debug("\t * Module @ %p \n", (void*)mod->mod_start);
      debug("\t * Args: %s \n ", (char*)mod->cmdline);
      debug("\t * End: %p \n ", (char*)mod->mod_end);

      if (mod->mod_end > multi_end)
        multi_end = mod->mod_end;

    }

    debug("* Multiboot end: 0x%x \n", multi_end);
    return multi_end;
  }
}

void OS::multiboot(uint32_t boot_magic, uint32_t boot_addr){
  MYINFO("Booted with multiboot");

  INFO2("* magic value: 0x%x Multiboot info at 0x%x", boot_magic, boot_addr);
  bootinfo_ = (multiboot_info_t*) boot_addr;

  if (! bootinfo_->flags & MULTIBOOT_INFO_MEMORY) {
    INFO2("* No memory info provided in multiboot info");
    return;
  }

  uint32_t mem_low_start = 0;
  uint32_t mem_low_end = (bootinfo_->mem_lower * 1024) - 1;
  uint32_t mem_low_kb = bootinfo_->mem_lower;
  uint32_t mem_high_start = 0x100000;
  uint32_t mem_high_end = mem_high_start + (bootinfo_->mem_upper * 1024) - 1;
  uint32_t mem_high_kb = bootinfo_->mem_upper;

  OS::low_memory_size_ = mem_low_kb * 1024;
  OS::high_memory_size_ = mem_high_kb * 1024;
  OS::memory_end_ = high_memory_size_ + mem_high_start;

  INFO2("* Valid memory (%i Kib):", mem_low_kb + mem_high_kb);
  INFO2("\t 0x%08x - 0x%08x (%i Kib)",
        mem_low_start, mem_low_end, mem_low_kb);
  INFO2("\t 0x%08x - 0x%08x (%i Kib)",
        mem_high_start, mem_high_end, mem_high_kb);
  INFO2("");

  if (bootinfo_->flags & MULTIBOOT_INFO_CMDLINE) {
    INFO2("* Booted with parameters @ 0x%x: %s", bootinfo_->cmdline,
          reinterpret_cast<const char*>(bootinfo_->cmdline));
    cmdline = reinterpret_cast<const char*>(bootinfo_->cmdline);
  }

  if (bootinfo_->flags & MULTIBOOT_INFO_MEM_MAP) {
    INFO2("* Multiboot provided memory map  (%i entries @ %p)",
          bootinfo_->mmap_length / sizeof(multiboot_memory_map_t), (void*)bootinfo_->mmap_addr);
    gsl::span<multiboot_memory_map_t> mmap { reinterpret_cast<multiboot_memory_map_t*>(bootinfo_->mmap_addr),
        (int)(bootinfo_->mmap_length / sizeof(multiboot_memory_map_t))};

    for (auto map : mmap) {
      const char* str_type = map.type & MULTIBOOT_MEMORY_AVAILABLE ? "FREE" : "RESERVED";
      INFO2("\t 0x%08llx - 0x%08llx %s (%llu Kb.)",
            map.addr, map.addr + map.len - 1, str_type, map.len / 1024 );

      if (not (map.type & MULTIBOOT_MEMORY_AVAILABLE)) {
        memory_map().assign_range({static_cast<uintptr_t>(map.addr),
              static_cast<uintptr_t>(map.addr + map.len - 1), "Reserved", "Multiboot / BIOS"});
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
