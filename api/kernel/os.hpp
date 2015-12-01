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

#ifndef KERNEL_OS_HPP
#define KERNEL_OS_HPP

#ifndef OS_VERSION
#define OS_VERSION "v?.?.?"
#endif

#include <common>
#include <string>
#include "../hw/pit.hpp"

/** The entrypoint for OS services
    
    @note For device access, see Dev
 */
class OS{
  
 public:     
  typedef delegate<void(const char*, size_t)> rsprint_func;
  
  // No copy or move
  OS(OS&) = delete;
  OS(OS&&) = delete;
  
  // No construction
  OS() = delete;

  static inline std::string version(){
    return std::string(OS_VERSION);
  }
  
  /** Clock cycles since boot. */
  static inline uint64_t cycles_since_boot()
  {
    uint64_t ret;
    __asm__ volatile ("rdtsc":"=A"(ret));
    return ret;
  }
  
  /** Uptime in seconds. */
  static double uptime();
  
  /** Receive a byte from port. @todo Should be moved 
      @param port : The port number to receive from
  */
  static uint8_t inb(int port);
  
  /** Send a byte to port. @todo Should be moved to hw/...something 
      @param port : The port to send to
	  @param data : One byte of data to send to @param port
  */
  static void outb(int port, uint8_t data);
    
  /** Write a cstring to serial port. @todo Should be moved to Dev::serial(n).
      @param ptr : the string to write to serial port
  */
  static size_t rsprint(const char* ptr);
  static size_t rsprint(const char* ptr, size_t len);
  
  /** Write a character to serial port. @todo Should be moved Dev::serial(n) 
      @param c : The character to print to serial port
  */
  static void rswrite(char c);

  /** Start the OS.  @todo Should be `init()` - and not accessible from ABI */
  static void start();

  
  /** Halt until next inerrupt. 
      @Warning If there is no regular timer interrupt (i.e. from PIT / APIC) 
      we'll stay asleep. 
   */
  static void halt();  
  
  /** Set handler for secondary serial output.
      This handler is called in addition to writing to the serial port.
   */
  static void set_rsprint_secondary(rsprint_func func)
  {
    rsprint_handler = func;
  }
  
private:  
  
  /** Indicate if the OS is running. */
  static bool _power;
  
  /** The main event loop.  Check interrupts, timers etc., and do callbacks. */
  static void event_loop();
  
  static MHz _CPU_mhz;
  
  static rsprint_func rsprint_handler;
};


#endif
