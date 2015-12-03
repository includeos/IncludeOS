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
#define MYINFO(X,...) INFO("Kernel",X,##__VA_ARGS__)
#include <os>
#include <stdio.h>
#include <assert.h>

// A private class to handle IRQ
#include <kernel/irq_manager.hpp>
#include <hw/pci_manager.hpp>
#include <service>
#include <stdlib.h>

bool OS::_power = true;
MHz  OS::_CPU_mhz(0);
// we have to initialize delegates?
OS::rsprint_func OS::rsprint_handler =
  [](const char*, size_t) {};

extern "C" uint16_t _cpu_sampling_freq_divider_;

void OS::start()
{
  debug("  * OS class started\n");
  srand(time(NULL));
  
  // heap
  extern caddr_t heap_end;
  extern char    _end;
  MYINFO("Heap start: %p", heap_end);
  MYINFO("Current end is: %p", &_end);
  
  //OS::rsprint("  * IRQ handler\n");
  asm("cli");  
  IRQ_manager::init();
  
  // Initialize the Interval Timer
  PIT::init();

  // Initialize PCI devices
  PCI_manager::init();

  asm("sti");
  
  
  MYINFO("Estimating CPU-frequency");
  INFO2("|");
  INFO2("+--(10 samples, %f sec. interval)", 
	(PIT::frequency() / _cpu_sampling_freq_divider_).count());
  INFO2("|");
  
  // TODO: Debug why actual measurments sometimes causes problems. Issue #246. 
  _CPU_mhz = MHz(2200); //PIT::CPUFrequency();

  INFO2("+--> %f MHz", _CPU_mhz.count());
    
  MYINFO("Starting %s",Service::name().c_str());
  FILLINE('=');
  // Everything is ready
  Service::start();
  
  event_loop();
}

void OS::halt(){
  __asm__ volatile("hlt;");
}

double OS::uptime(){  
  return (cycles_since_boot() / Hz(_CPU_mhz).count()) ; 
}

void OS::event_loop()
{
  FILLINE('=');
  printf(" IncludeOS %s \n",version().c_str());
  printf(" +--> Running [ %s ] \n", Service::name().c_str());
  FILLINE('~');
  
  while (_power)
  {
    IRQ_manager::notify(); 
    
    debug("<OS> Woke up @ t = %li \n",uptime());
  }
  
  //Cleanup
  //Service::stop();
}

size_t OS::rsprint(const char* str)
{
  size_t len = 0;
	// measure length
  while (str[len++]);
  
  // call rsprint again with length
  return rsprint(str, len);
}
size_t OS::rsprint(const char* str, size_t len)
{
	// serial output
	for(size_t i = 0; i < len; i++)
		rswrite(str[i]);
	
  // call external handler for secondary outputs
  OS::rsprint_handler(str, len);
	
	return len;
}

/* STEAL: Read byte from I/O address space */
uint8_t OS::inb(int port)
{
  int ret;
  __asm__ volatile ("xorl %eax,%eax");
  __asm__ volatile ("inb %%dx,%%al":"=a" (ret):"d"(port));

  return ret;
}

/*  Write byte to I/O address space */
void OS::outb(int port, uint8_t data) {
  __asm__ volatile ("outb %%al,%%dx"::"a" (data), "d"(port));
}

/* 
 * STEAL: Print to serial port 0x3F8
 */
void OS::rswrite(char c)
{
  /* Wait for the previous character to be sent */
  while ((inb(0x3FD) & 0x20) != 0x20);

  /* Send the character */
  outb(0x3F8, c);
}
