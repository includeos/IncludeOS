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

#include <kernel/os.hpp>
#include <kernel/cpuid.hpp>
#include <kernel/rng.hpp>
#include <kernel/syscalls.hpp>
#include <boot/multiboot.h>
#include <kprint>
#include <debug>
#include <util/elf_binary.hpp>
#include <arch/x86/cpu.hpp>
#include <kernel/auxvec.h>
#include <kernel/service.hpp>

#include "idt.hpp"

#undef Expects
#define Expects(X) if (!(X)) { kprint("Expect failed: " #X "\n");  asm("cli;hlt"); }

//#define KERN_DEBUG 1
#ifdef KERN_DEBUG
#define PRATTLE(fmt, ...) kprintf(fmt, ##__VA_ARGS__)
#else
#define PRATTLE(fmt, ...) /* fmt */
#endif

extern "C" {
  void __init_serial1();
  void __init_sanity_checks();
  void kernel_sanity_checks();
  void _init_bss();
  uintptr_t _move_symbols(uintptr_t loc);
  void _init_elf_parser();
  void _init_syscalls();
}

uintptr_t _multiboot_free_begin(uintptr_t bootinfo);
uintptr_t _multiboot_memory_end(uintptr_t bootinfo);

extern char _ELF_START_;
extern char _ELF_END_;
extern char _INIT_START_;
extern char _FINI_START_;

thread_local int __tl1__ = 42;

uint32_t __grub_magic = 0xc001;
uint32_t __grub_addr  = 0x7001;

static volatile int __global_ctors_ok = 0;

__attribute__((no_sanitize("all")))
void _init_bss()
{
  extern char _BSS_START_, _BSS_END_;
  __builtin_memset(&_BSS_START_, 0, &_BSS_END_ - &_BSS_START_);
}

__attribute__((constructor))
static void global_ctor_test(){
  __global_ctors_ok = 42;
}

int kernel_main(int, char * *, char * *) {
  PRATTLE("<kernel_main> libc initialization complete \n");
  Expects(__global_ctors_ok == 42);
  extern bool __libc_initialized;
  __libc_initialized = true;

  Expects(__tl1__ == 42);
  Elf_binary<Elf64> elf{{(char*)&_ELF_START_, &_ELF_END_ - &_ELF_START_}};
  Expects(elf.is_ELF() && "ELF header intact");

  PRATTLE("<kernel_main> OS start \n");
  // Initialize early OS, platform and devices
  OS::start(__grub_magic,__grub_addr);

  PRATTLE("<kernel_main> post start \n");
  // Initialize common subsystems and call Service::start
  OS::post_start();

  // verify certain read-only sections in memory
  kernel_sanity_checks();

  // Starting event loop from here allows us to profile OS::start
  OS::event_loop();
  return 0;
}

// Musl entry
extern "C"
int __libc_start_main(int (*main)(int,char **,char **), int argc, char **argv);

extern "C" uintptr_t __syscall_entry();
extern "C" void __elf_validate_section(const void*);

extern "C"
__attribute__((no_sanitize("all")))
void kernel_start(uint32_t magic, uint32_t addr)
{
  // Initialize default serial port
  __init_serial1();

  __grub_magic = magic;
  __grub_addr  = addr;

  PRATTLE("\n//////////////////  IncludeOS kernel start ////////////////// \n");
  PRATTLE("* Booted with magic 0x%x, grub @ 0x%x \n* Init sanity\n",
          magic, addr);
  // generate checksums of read-only areas etc.
  __init_sanity_checks();

  PRATTLE("* Grub magic: 0x%x, grub info @ 0x%x\n",
          __grub_magic, __grub_addr);

  // Determine where free memory starts
  extern char _end;
  uintptr_t free_mem_begin = reinterpret_cast<uintptr_t>(&_end);
  uintptr_t memory_end     = OS::memory_end();

  if (magic == MULTIBOOT_BOOTLOADER_MAGIC) {
    free_mem_begin = _multiboot_free_begin(addr);
    memory_end     = _multiboot_memory_end(addr);
  }
  else if (OS::is_softreset_magic(magic))
  {
    memory_end = OS::softreset_memory_end(addr);
  }
  PRATTLE("* Free mem begin: 0x%zx, memory end: 0x%zx \n",
          free_mem_begin, memory_end);

  PRATTLE("* Moving symbols. \n");
  // Preserve symbols from the ELF binary
  free_mem_begin += _move_symbols(free_mem_begin);
  PRATTLE("* Free mem moved to: %p \n", (void*) free_mem_begin);

  PRATTLE("* Grub magic: 0x%x, grub info @ 0x%x\n",
          __grub_magic, __grub_addr);

  PRATTLE("* Init .bss\n");
  _init_bss();

  PRATTLE("* Init heap\n");
  OS::init_heap(free_mem_begin, memory_end);

  PRATTLE("* Init syscalls\n");
  _init_syscalls();

  PRATTLE("* Init CPU exceptions\n");
  x86::idt_initialize_for_cpu(0);

  PRATTLE("* Init ELF parser\n");
  _init_elf_parser();

  PRATTLE("* Thread local1: %i\n", __tl1__);

  PRATTLE("* Elf start: %p\n", &_ELF_START_);
  auto* ehdr = (Elf64_Ehdr*)&_ELF_START_;
  auto* phdr = (Elf64_Phdr*)((char*)ehdr + ehdr->e_phoff);
  Expects(phdr);
  Elf_binary<Elf64> elf{{(char*)&_ELF_START_, &_ELF_END_ - &_ELF_START_}};
  Expects(elf.is_ELF());
  Expects(phdr[0].p_type == PT_LOAD);

#ifdef KERN_DEBUG
  PRATTLE("* Elf ident: %s, program headers: %p\n", ehdr->e_ident, ehdr);
  size_t size =  &_ELF_END_ - &_ELF_START_;
  PRATTLE("\tElf size: %zu \n", size);
  for (int i = 0; i < ehdr->e_phnum; i++)
  {
    PRATTLE("\tPhdr %i @ %p, va_addr: 0x%lx \n", i, &phdr[i], phdr[i].p_vaddr);
  }
#endif

  // initialize RNG as early as possible
  RNG::init();

  // Build AUX-vector for C-runtime
  std::array<char*, 6 + 38*2> argv;
  // Parameters to main
  argv[0] = (char*) Service::name();
  argv[1] = 0x0;
  int argc = 1;

  // Env vars
  argv[2] = strdup("LC_CTYPE=C");
  argv[3] = strdup("LC_ALL=C");
  argv[4] = strdup("USER=root");
  argv[5] = 0x0;

  // auxiliary vector
  auxv_t* aux = (auxv_t*) &argv[6];
  PRATTLE("* Initializing aux-vector @ %p\n", aux);

  int i = 0;
  aux[i++].set_long(AT_PAGESZ, 4096);
  aux[i++].set_long(AT_CLKTCK, 100);

  // ELF related
  aux[i++].set_long(AT_PHENT, ehdr->e_phentsize);
  aux[i++].set_ptr(AT_PHDR, ((uint8_t*)ehdr) + ehdr->e_phoff);
  aux[i++].set_long(AT_PHNUM, ehdr->e_phnum);

  // Misc
  aux[i++].set_ptr(AT_BASE, nullptr);
  aux[i++].set_long(AT_FLAGS, 0x0);
  aux[i++].set_ptr(AT_ENTRY, (void*) &kernel_main);
  aux[i++].set_long(AT_HWCAP, 0);
  aux[i++].set_long(AT_UID, 0);
  aux[i++].set_long(AT_EUID, 0);
  aux[i++].set_long(AT_GID, 0);
  aux[i++].set_long(AT_EGID, 0);
  aux[i++].set_long(AT_SECURE, 1);

  const char* plat = "x86_64";
  aux[i++].set_ptr(AT_PLATFORM, plat);

  // SSP value generated from system RNG
  const long canary = rng_extract_uint64();
  const long canary_idx = i;
  aux[i++].set_long(AT_RANDOM, canary);
  kprintf("* Stack protector value: %#lx\n", canary);
  // entropy slot
  aux[i++].set_ptr(AT_RANDOM, &aux[canary_idx].a_un.a_val);
  aux[i++].set_long(AT_NULL, 0);

#if defined(__x86_64__)
  PRATTLE("* Initialize syscall MSR (64-bit)\n");
  uint64_t star_kernel_cs = 8ull << 32;
  uint64_t star_user_cs   = 8ull << 48;
  uint64_t star = star_kernel_cs | star_user_cs;
  x86::CPU::write_msr(IA32_STAR, star);
  x86::CPU::write_msr(IA32_LSTAR, (uintptr_t)&__syscall_entry);
#elif defined(__i386__)
  PRATTLE("Initialize syscall intr (32-bit)\n");
  #warning Classical syscall interface missing for 32-bit
#endif

  // GDB_ENTRY;
  PRATTLE("* Starting libc initialization\n");
  __libc_start_main(kernel_main, argc, argv.data());
}
