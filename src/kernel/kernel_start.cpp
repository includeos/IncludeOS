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

extern "C" void _start() __attribute__((visibility("hidden"))) __attribute__ ((noreturn));
extern "C" void _init_c_runtime();
extern "C" void _init_syscalls();

// enables Streaming SIMD Extensions
static void enableSSE(void)
{
  asm ("mov %cr0, %eax");
  asm ("and $0xFFFB,%ax");
  asm ("or  $0x2,   %ax");
  asm ("mov %eax, %cr0");

  asm ("mov %cr4, %eax");
  asm ("or  $0x600,%ax");
  asm ("mov %eax, %cr4");
}


extern "C" void kernel_start(uintptr_t magic, uintptr_t addr)  {

  // enable SSE extensions bitmask in CR4 register
  enableSSE();

  // Initialize system calls
  _init_syscalls();

  // Initialize stack-unwinder, call global constructors etc.
  _init_c_runtime();

  // Initialize OS including devices
  OS::start(magic, addr);
}
