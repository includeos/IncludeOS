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
#define MYINFO(X,...) INFO("Kernel", X, ##__VA_ARGS__)

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include <os>

// A private class to handle IRQ
#include <hw/ioport.hpp>
#include <kernel/pci_manager.hpp>
#include <kernel/irq_manager.hpp>

bool OS::power_   {true};
MHz  OS::cpu_mhz_ {0};

// We have to initialize delegates?
OS::rsprint_func OS::rsprint_handler_ =
  [](const char*, size_t) {};

extern "C" uint16_t _cpu_sampling_freq_divider_;

void OS::start() {
  debug("\t[*] OS class started\n");
  srand(time(NULL));
  
  // Heap
  extern caddr_t heap_end;
  extern char    _end;

  MYINFO("Heap start: @ %p", heap_end);
  MYINFO("Current end is: @ %p", &_end);
  
  // OS::rsprint("\t[*] IRQ handler\n");

  asm("cli");

  IRQ_manager::init();
  
  // Initialize the Interval Timer
  hw::PIT::init();

  // Initialize PCI devices
  PCI_manager::init();

  asm("sti");
  
  
  MYINFO("Estimating CPU-frequency");
  INFO2("|");
  INFO2("+--(10 samples, %f sec. interval)", 
	(hw::PIT::frequency() / _cpu_sampling_freq_divider_).count());
  INFO2("|");
  
  // TODO: Debug why actual measurments sometimes causes problems. Issue #246.
  cpu_mhz_ = MHz(2200); //hw::PIT::CPUFrequency();

  INFO2("+--> %f MHz", cpu_mhz_.count());
    
  MYINFO("Starting %s", Service::name().c_str());
  FILLINE('=');

  // Everything is ready
  Service::start();
  
  event_loop();
}

void OS::halt() {
  __asm__ volatile("hlt;");
}

double OS::uptime() {
  return (cycles_since_boot() / Hz(cpu_mhz_).count());
}

void OS::event_loop() {
  FILLINE('=');
  printf(" IncludeOS %s\n", version().c_str());
  printf(" +--> Running [ %s ]\n", Service::name().c_str());
  FILLINE('~');
  
  while (power_) {
    IRQ_manager::notify(); 
    debug("<OS> Woke up @ t = %li\n", uptime());
  }
  
  //Cleanup
  //Service::stop();
}

size_t OS::rsprint(const char* str) {
  size_t len {0};

	// Measure length
  while (str[len++]);
  
  // Call rsprint again with length
  return rsprint(str, len);
}

size_t OS::rsprint(const char* str, const size_t len) {
	// Serial output
	for(size_t i {0}; i < len; ++i)
		rswrite(str[i]);
	
  // Call external handler for secondary outputs
  OS::rsprint_handler_(str, len);
	
	return len;
}

/* STEAL: Print to serial port 0x3F8 */
void OS::rswrite(const char c) {
  /* Wait for the previous character to be sent */
  while ((hw::inb(0x3FD) & 0x20) != 0x20);

  /* Send the character */
  hw::outb(0x3F8, c);
}
