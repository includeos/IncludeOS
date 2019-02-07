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

#include <os.hpp>
#include <boot/multiboot.h>
#include <util/elf_binary.hpp>
#include <service>
#include <cstdint>
#include <cstring>

extern bool os_enable_boot_logging;

#define MYINFO(X,...) \
  if (os_enable_boot_logging) printf("[ chainloader ] " X "\n",  ##__VA_ARGS__);

extern "C" void hotswap(const char* base, int len, char* dest, void* start,
                        uintptr_t magic, uintptr_t bootinfo);

extern uint32_t __multiboot_magic;
extern uint32_t __multiboot_addr;

/** Modify multiboot data to show first module as the kernel */
void promote_mod_to_kernel()
{
  auto* bootinfo = (multiboot_info_t*) (uintptr_t) __multiboot_addr;

  Expects (bootinfo->mods_count);
  auto* mod =  (multiboot_module_t*)bootinfo->mods_addr;

  // Move commandline to a relatively safe area
  const uintptr_t RELATIVELY_SAFE_AREA = 0x8000;
  strcpy((char*) RELATIVELY_SAFE_AREA, (const char*) mod->cmdline);
  bootinfo->cmdline = RELATIVELY_SAFE_AREA;

  // Subtract one module
  (bootinfo->mods_count)--;

  if (bootinfo->mods_count)
    bootinfo->mods_addr = (uintptr_t)((multiboot_module_t*)bootinfo->mods_addr + 1);
}

void Service::start()
{

  auto mods = os::modules();
  MYINFO("%u-bit chainloader found %u modules",
        sizeof(void*) * 8, mods.size());

  if (mods.size() < 1) {
    MYINFO("No modules passed to multiboot. Exiting.");
    exit(1);
  }

  auto binary = mods[0];

  Elf_binary<Elf64> elf (
      {(char*)binary.mod_start,
        (int)(binary.mod_end - binary.mod_start)});


  auto phdrs = elf.program_headers();
  int loadable = 0;

  for (auto& phdr : phdrs){
    if (phdr.p_type == PT_LOAD)
      loadable++;
  }

  auto init_seg = phdrs[0];
  // Expects(loadable == 1);
  // TODO: Handle multiple loadable segments properly
  Expects(init_seg.p_type == PT_LOAD);

  // Move hotswap function away from binary
  void* hotswap_addr = (void*)0x2000;
  extern char __hotswap_end;
  memcpy(hotswap_addr,(void*)&hotswap, &__hotswap_end - (char*)&hotswap );

  MYINFO("Preparing for jump to %s. Multiboot magic: 0x%x, addr 0x%x",
         (char*)binary.params, __multiboot_magic, __multiboot_addr);

  // Prepare to load ELF segment
  char* base  = (char*)binary.mod_start + init_seg.p_offset;
  int len = (int)((char*)binary.mod_end - base);
  char* dest = (char*) init_seg.p_paddr;
  void* start = (void*) elf.entry();

  // Update multiboot info for new kernel
  promote_mod_to_kernel();

  // Clear interrupts
  asm("cli");

  // Call hotswap, overwriting current kernel
  ((decltype(&hotswap))hotswap_addr)(base, len, dest, start, __multiboot_magic, __multiboot_addr);

  os::panic("Should have jumped\n");

  __builtin_unreachable();
}
