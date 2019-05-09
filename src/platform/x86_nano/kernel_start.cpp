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

#include <kprint>
#include <info>
#include <kernel.hpp>
#include <os.hpp>
#include <kernel/service.hpp>
#include <boot/multiboot.h>

extern "C" {
  void __init_sanity_checks();
  uintptr_t _move_symbols(uintptr_t loc);
  void _init_bss();
  void _init_heap(uintptr_t);
  void _init_syscalls();
}

uintptr_t _multiboot_free_begin(uintptr_t boot_addr);
uintptr_t _multiboot_memory_end(uintptr_t boot_addr);
extern bool os_default_stdout;

extern "C"
void kernel_start(uintptr_t magic, uintptr_t addr)
{
  // Determine where free memory starts
  extern char _end;
  uintptr_t free_mem_begin = (uintptr_t) &_end;
  uintptr_t mem_end = os::Arch::max_canonical_addr;

  if (magic == MULTIBOOT_BOOTLOADER_MAGIC) {
    free_mem_begin = _multiboot_free_begin(addr);
    mem_end = _multiboot_memory_end(addr);
  }

  // Preserve symbols from the ELF binary
  free_mem_begin += _move_symbols(free_mem_begin);

  // Initialize .bss
  extern char _BSS_START_, _BSS_END_;
  __builtin_memset(&_BSS_START_, 0, &_BSS_END_ - &_BSS_START_);

  // Initialize heap
  kernel::init_heap(free_mem_begin, mem_end);

  // Initialize system calls
  _init_syscalls();

  // Initialize stdout handlers
  if (os_default_stdout)
    os::add_stdout(&kernel::default_stdout);

  kernel::start(magic, addr);

  // Start the service
  Service::start();

  __arch_poweroff();
}

/*
extern "C" int __divdi3() {}
extern "C" int __moddi3() {}
extern "C" unsigned int __udivdi3() {}
extern "C" unsigned int __umoddi3() {}
*/
