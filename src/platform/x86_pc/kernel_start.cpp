// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2018 IncludeOS AS, Oslo, Norway
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

#include <kernel.hpp>
#include <os.hpp>
#include <boot/multiboot.h>
#include <kprint>
#include <debug>

#include "idt.hpp"
#include "init_libc.hpp"

//#define KERN_DEBUG 1
#ifdef KERN_DEBUG
#define PRATTLE(fmt, ...) kprintf(fmt, ##__VA_ARGS__)
#else
#define PRATTLE(fmt, ...) /* fmt */
#endif

extern "C" {
  void __init_sanity_checks();
  void kernel_sanity_checks();
  void _init_bss();
  uintptr_t _move_symbols(uintptr_t loc);
  void _init_elf_parser();
  void _init_syscalls();
  void __elf_validate_section(const void*);
}

uintptr_t _multiboot_free_begin(uintptr_t bootinfo);
uintptr_t _multiboot_memory_end(uintptr_t bootinfo);

__attribute__((no_sanitize("all")))
void _init_bss()
{
  extern char _BSS_START_, _BSS_END_;
  __builtin_memset(&_BSS_START_, 0, &_BSS_END_ - &_BSS_START_);
}

static os::Machine* __machine = nullptr;
os::Machine& os::machine() noexcept {
  LL_ASSERT(__machine != nullptr);
  return *__machine;
}

// x86 kernel start
extern "C"
__attribute__((no_sanitize("all")))
void kernel_start(uint32_t magic, uint32_t addr)
{
  PRATTLE("\n//////////////////  IncludeOS kernel start ////////////////// \n");
  PRATTLE("* Booted with magic 0x%x, grub @ 0x%x \n",
          magic, addr);
  // generate checksums of read-only areas etc.
  __init_sanity_checks();

  PRATTLE("* Grub magic: 0x%x, grub info @ 0x%x\n", magic, addr);

  // Determine where free memory starts
  extern char _end;
  uintptr_t free_mem_begin = reinterpret_cast<uintptr_t>(&_end);
  uintptr_t memory_end     = kernel::memory_end();

  if (magic == MULTIBOOT_BOOTLOADER_MAGIC) {
    free_mem_begin = _multiboot_free_begin(addr);
    memory_end     = _multiboot_memory_end(addr);
  }
  else if (kernel::is_softreset_magic(magic))
  {
    memory_end = kernel::softreset_memory_end(addr);
  }
  PRATTLE("* Free mem begin: 0x%zx, memory end: 0x%zx \n",
          free_mem_begin, memory_end);

  PRATTLE("* Moving symbols. \n");
  // Preserve symbols from the ELF binary
  free_mem_begin += _move_symbols(free_mem_begin);
  PRATTLE("* Free mem moved to: %p \n", (void*) free_mem_begin);

  PRATTLE("* Init .bss\n");
  _init_bss();

  // Instantiate machine
  size_t memsize = memory_end - free_mem_begin;
  __machine = os::Machine::create((void*)free_mem_begin, memsize);

  PRATTLE("* Init ELF parser\n");
  _init_elf_parser();

  // Begin portable HAL initialization
  __machine->init();

  // TODO: Move more stuff into Machine::init

  PRATTLE("* Init syscalls\n");
  _init_syscalls();

  PRATTLE("* Init CPU exceptions\n");
  x86::idt_initialize_for_cpu(0);

  x86::init_libc(magic, addr);
}
