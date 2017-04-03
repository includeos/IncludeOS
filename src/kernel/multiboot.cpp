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
#include <boot/multiboot.h>

#define MYINFO(X,...) INFO("Kernel", X, ##__VA_ARGS__)

void OS::multiboot(uint32_t boot_magic, uint32_t boot_addr){
  MYINFO("Booted with multiboot");
  INFO2("* magic value: 0x%x Multiboot info at 0x%x", boot_magic, boot_addr);

  multiboot_info_t* bootinfo = (multiboot_info_t*) boot_addr;

  if (! bootinfo->flags & MULTIBOOT_INFO_MEMORY) {
    INFO2("* No memory info provided in multiboot info");
    return;
  }

  uint32_t mem_low_start = 0;
  uint32_t mem_low_end = (bootinfo->mem_lower * 1024) - 1;
  uint32_t mem_low_kb = bootinfo->mem_lower;
  uint32_t mem_high_start = 0x100000;
  uint32_t mem_high_end = mem_high_start + (bootinfo->mem_upper * 1024) - 1;
  uint32_t mem_high_kb = bootinfo->mem_upper;

  OS::low_memory_size_ = mem_low_kb * 1024;
  OS::high_memory_size_ = mem_high_kb * 1024;
  OS::memory_end_ = high_memory_size_ + mem_high_start;

  INFO2("* Valid memory (%i Kib):", mem_low_kb + mem_high_kb);
  INFO2("\t 0x%08x - 0x%08x (%i Kib)",
        mem_low_start, mem_low_end, mem_low_kb);
  INFO2("\t 0x%08x - 0x%08x (%i Kib)",
        mem_high_start, mem_high_end, mem_high_kb);
  INFO2("");

  if (bootinfo->flags & MULTIBOOT_INFO_CMDLINE) {
    INFO2("* Booted with parameters @ %p: %s",(void*)bootinfo->cmdline, (char*)bootinfo->cmdline);
    cmdline = std::string((char*) bootinfo->cmdline);
  }

  if (bootinfo->flags & MULTIBOOT_INFO_MEM_MAP) {
    INFO2("* Multiboot provided memory map  (%i entries @ %p)",
          bootinfo->mmap_length / sizeof(multiboot_memory_map_t), (void*)bootinfo->mmap_addr);
    gsl::span<multiboot_memory_map_t> mmap { reinterpret_cast<multiboot_memory_map_t*>(bootinfo->mmap_addr),
        (int)(bootinfo->mmap_length / sizeof(multiboot_memory_map_t))};

    for (auto map : mmap) {
      const char* str_type = map.type & MULTIBOOT_MEMORY_AVAILABLE ? "FREE" : "RESERVED";
      INFO2("\t 0x%08llx - 0x%08llx %s (%llu Kb.)",
            map.addr, map.addr + map.len - 1, str_type, map.len / 1024 );

      if (not (map.type & MULTIBOOT_MEMORY_AVAILABLE)) {
        memory_map().assign_range({static_cast<uintptr_t>(map.addr), static_cast<uintptr_t>(map.addr + map.len - 1), "Reserved", "Multiboot / BIOS"});
      }
    }
    printf("\n");
  }
}
