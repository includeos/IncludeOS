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
#include <kernel/os.hpp>
#include <kernel/service.hpp>
#include <boot/multiboot.h>

extern  void __platform_init();

extern "C" {
  void __init_serial1();
  void __init_sanity_checks();
  uintptr_t _multiboot_free_begin(uintptr_t boot_addr);
  uintptr_t _move_symbols(uintptr_t loc);
  void _init_bss();
  void _init_heap(uintptr_t);
  void _init_c_runtime();
  void _init_syscalls();
  void __libc_init_array();
  uintptr_t _end;
}

extern "C"
void kernel_start(uintptr_t magic, uintptr_t addr)
{
  // Initialize serial port 1
  __init_serial1();

  // Determine where free memory starts
  uintptr_t free_mem_begin = reinterpret_cast<uintptr_t>(&_end);

  if (magic == MULTIBOOT_BOOTLOADER_MAGIC) {
    free_mem_begin = _multiboot_free_begin(addr);
  }

  // Preserve symbols from the ELF binary
  free_mem_begin += _move_symbols(free_mem_begin);

  // Initialize zero-initialized vars
  _init_bss();

  // Initialize heap
  _init_heap(free_mem_begin);

  //Initialize stack-unwinder, call global constructors etc.
  _init_c_runtime();

  // Initialize system calls
  _init_syscalls();

  //Initialize stdout handlers
  OS::add_stdout(&OS::default_stdout);

  // Call global ctors
  __libc_init_array();

  __platform_init();

  // Start the service
  Service::start();

  __arch_poweroff();
}
