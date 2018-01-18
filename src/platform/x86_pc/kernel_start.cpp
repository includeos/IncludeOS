// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
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
#include <kernel/cpuid.hpp>
#include <boot/multiboot.h>
#include <kprint>
#include "musl_init.cpp"

extern "C" {
  void __init_serial1();
  void __init_sanity_checks();
  void kernel_sanity_checks();
  uintptr_t _multiboot_free_begin(uintptr_t boot_addr);
  //uintptr_t _move_symbols(uintptr_t loc);
}

extern uintptr_t heap_begin;
extern uintptr_t heap_end;

void _init_heap(uintptr_t free_mem_begin)
{
  #define HEAP_ALIGNMENT   4095
  // NOTE: Initialize the heap before exceptions
  // cache-align heap, because its not aligned
  heap_begin = free_mem_begin + HEAP_ALIGNMENT;
  heap_begin = ((uintptr_t)heap_begin & ~HEAP_ALIGNMENT);
  heap_end   = heap_begin;
}

int kernel_main(int, char * *, char * *) {
  kprintf("kernel_main() !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
  // Initialize early OS, platform and devices
  OS::start(0u,0u);

  // Initialize common subsystems and call Service::start
  OS::post_start();

  // verify certain read-only sections in memory
  kernel_sanity_checks();

  // Starting event loop from here allows us to profile OS::start
  OS::event_loop();
}

void __init_tls(size_t* p)
{
  kprintf("init_tls(%p)\n", p);
}

extern "C"
int __libc_start_main(int *(main) (int, char * *, char * *),
                     int argc, char * * ubp_av,
                     void (*init) (void),
                     void (*fini) (void),
                     void (*rtld_fini) (void),
                     void (* stack_end));

extern "C" void _init();
extern "C" void _fini();
#include <kernel/auxvec.h>
#include <kernel/service.hpp>

extern "C"
__attribute__((no_sanitize("all")))
void kernel_start(uintptr_t magic, uintptr_t addr)
{
  // Initialize default serial port
  __init_serial1();

  // generate checksums of read-only areas etc.
  __init_sanity_checks();

  // Determine where free memory starts
  extern char _end;
  uintptr_t free_mem_begin = reinterpret_cast<uintptr_t>(&_end);

  if (magic == MULTIBOOT_BOOTLOADER_MAGIC) {
    free_mem_begin = _multiboot_free_begin(addr);
  }

  // Preserve symbols from the ELF binary
  //free_mem_begin += _move_symbols(free_mem_begin);

  // TODO: set heap begin
  _init_heap(free_mem_begin);

  extern char _INIT_START_;
  extern char _FINI_START_;
  void* init_location = &_INIT_START_;
  void* fini_location = &_FINI_START_;
  void* rtld_fini = nullptr;
  void* stack_end = (void*) 0x10000;

  auxv_t aux[38];
  int i = 0;
  aux[i++].set_long(AT_PAGESZ, 4096);
  aux[i++].set_long(AT_CLKTCK, 100);

  aux[i++].set_long(AT_PHENT, 32);
  aux[i++].set_ptr(AT_PHDR, 0x0);
  aux[i++].set_long(AT_PHNUM, 0);
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

  aux[i++].set_long(AT_NULL, 0);

  std::array<char*, 4 + 38> argv;
  // arguments to "program"
  argv[0] = (char*) Service::name();
  argv[1] = 0x0;
  int argc = 1;

  // "environment" variables
  argv[2] = "BLARGH=0";
  argv[3] = 0x0;

  memcpy(&argv[4], aux, sizeof(auxv_t) * 38);

  // ubp_av = argv, irrelevant when argc = 0
  hallo__libc_start_main(kernel_main, argc, argv.data());

}
