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
#include <debug>

extern "C"
{
  extern void _init_c_runtime();
  static void init_serial();
  
#ifdef DEBUG
  static const int _test_glob = 123;
  static int _test_constructor = 0;
#endif
  
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
  
#ifdef DEBUG
  __attribute__((constructor)) void test_constr()
  {    
    OS::rsprint("\t * C constructor was called!\n");
    _test_constructor = 1;
  }
#endif
  
  void _start(void)
  {    
    __asm__ volatile ("cli");
    
    // enable SSE extensions bitmask in CR4 register
    enableSSE();
    
    // init serial port
    init_serial();    
    
    // Initialize stack-unwinder, call global constructors etc.
    #ifdef DEBUG
      OS::rsprint("\t * Initializing C-environment... \n");
    #endif
    _init_c_runtime();
    
    FILLINE('=');
    CAPTION("#include<os> // Literally");
    FILLINE('=');
    
// verify that global constructors were called
#ifdef DEBUG
    assert(_test_glob == 123);
    assert(_test_constructor == 1);
#endif
    // Initialize some OS functionality
    OS::start();
    
    // Will only work if any destructors are called (I think?)
    //_fini();
  }
  
  #define SERIAL_PORT 0x3f8  
  void init_serial() {
    OS::outb(SERIAL_PORT + 1, 0x00);    // Disable all interrupts
    OS::outb(SERIAL_PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    OS::outb(SERIAL_PORT + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    OS::outb(SERIAL_PORT + 1, 0x00);    //                  (hi byte)
    OS::outb(SERIAL_PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
    OS::outb(SERIAL_PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    OS::outb(SERIAL_PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
  }
  
  int is_transmit_empty() {
    return OS::inb(SERIAL_PORT + 5) & 0x20;
  }
  
  void write_serial(char a) {
    while (is_transmit_empty() == 0);
    
    OS::outb(SERIAL_PORT, a);
  }
}
