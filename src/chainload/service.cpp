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

#include <kernel/os.hpp>
#include <kernel/syscalls.hpp>
#include <util/elf_binary.hpp>
#include <service>
#include <cstdint>

static const bool verb = false;
#define MYINFO(X,...) \
  if (verb) { INFO("chainload", X, ##__VA_ARGS__); }

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

  // Set command line param to mod param
  bootinfo->cmdline = mod->cmdline;

  // Subtract one module
  (bootinfo->mods_count)--;

  if (bootinfo->mods_count)
    bootinfo->mods_addr = (uintptr_t)((multiboot_module_t*)bootinfo->mods_addr + 1);
}

void Service::start()
{
  auto mods = OS::modules();
  MYINFO("%u-bit chainloader found %u modules",
        sizeof(void*) * 8, mods.size());

  if (mods.size() <= 0) {
    MYINFO("No modules passed to multiboot. Exiting.");
    exit(1);
  }
  multiboot_module_t binary = mods[0];

  Elf_binary<Elf64> elf (
      {(char*)binary.mod_start,
        (int)(binary.mod_end - binary.mod_start)});

  void* hotswap_addr = (void*)0x2000;
  extern char __hotswap_end;

  MYINFO("Moving hotswap function (begin at %p end at %p) of size %i",
         &hotswap,  &__hotswap_end, &__hotswap_end - (char*)&hotswap);
  memcpy(hotswap_addr,(void*)&hotswap, &__hotswap_end - (char*)&hotswap );

  MYINFO("Preparing for jump to %s. Multiboot magic: 0x%x, addr 0x%x",
         (char*)binary.cmdline, __multiboot_magic, __multiboot_addr);

  char* base  = (char*)binary.mod_start;
  int len = (int)(binary.mod_end - binary.mod_start);
  char* dest = (char*) 0xA00000; //elf.program_header().p_paddr;
  void* start = (void*) elf.entry();

  MYINFO("Hotswapping with params: base: %p, len: %i, dest: %p, start: %p",
         base, len, dest, start);

  promote_mod_to_kernel();

  asm("cli");
  ((decltype(&hotswap))hotswap_addr)(base, len, dest, start, __multiboot_magic, __multiboot_addr);

  panic("Should have jumped\n");
  __builtin_unreachable();
}
