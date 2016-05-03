// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
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

//#define DEBUG
#include <kernel/os.hpp>
#include <assert.h>

extern "C"
{
  extern uintptr_t __stack_chk_guard;
  void _init_c_runtime();
  
  // enables Streaming SIMD Extensions
  static void enableSSE(void)
  {
    __asm__ ("mov %cr0, %eax");
    __asm__ ("and $0xFFFB,%ax");
    __asm__ ("or  $0x2,   %ax");
    __asm__ ("mov %eax, %cr0");
    
    __asm__ ("mov %cr4, %eax");
    __asm__ ("or  $0x600,%ax");
    __asm__ ("mov %eax, %cr4");
  }
  
  static char __attribute__((noinline))
  stack_smasher(const char* src) {
    char bullshit[16];
    
    for (int i = -100; i < 100; i++)
      strcpy(bullshit+i, src);
    
    return bullshit[15];
  }
  
  void _start(void) {
    // enable SSE extensions bitmask in CR4 register
    enableSSE();
    
    //stack_smasher("1234567890 12345 hello world! test -.-");
    
    // Initialize stack-unwinder, call global constructors etc.
    _init_c_runtime();
    
    // Initialize some OS functionality
    OS::start();
  }
}
