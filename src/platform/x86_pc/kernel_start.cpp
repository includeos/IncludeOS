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

extern "C" {
  void __init_serial1();
  void __init_sanity_checks();
  void kernel_sanity_checks();
  uintptr_t _multiboot_free_begin(uintptr_t boot_addr);
  //uintptr_t _move_symbols(uintptr_t loc);
}

int* kernel_main(int, char * *, char * *) {
  // Initialize early OS, platform and devices
  OS::start(0u,0u);

  // Initialize common subsystems and call Service::start
  OS::post_start();

  // verify certain read-only sections in memory
  kernel_sanity_checks();

  // Starting event loop from here allows us to profile OS::start
  OS::event_loop();
}

typedef struct
{
  long int a_type;              /* Entry type */
  union
    {
      long int a_val;           /* Integer value */
      void *a_ptr;              /* Pointer value */
      void (*a_fcn) (void);     /* Function pointer value */
    } a_un;
} auxv_t;

extern "C"
int __libc_start_main(int *(main) (int, char * *, char * *),
                     int argc, char * * ubp_av,
                     void (*init) (void),
                     void (*fini) (void),
                     void (*rtld_fini) (void),
                     void (* stack_end));

extern "C" void _init();
extern "C" void _fini();

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

  extern char _INIT_START_;
  extern char _FINI_START_;
  void* init_location = &_INIT_START_;
  void* fini_location = &_FINI_START_;
  void* rtld_fini = nullptr;
  void* stack_end = (void*) 0x10000;

  __libc_start_main(kernel_main, 0, nullptr,
      _init, _fini, [](){}, stack_end);

}
