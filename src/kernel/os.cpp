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

#include <hw/acpi.hpp>
#include <hw/apic.hpp>
#include <hw/serial.hpp>
#include <kernel/pci_manager.hpp>
#include <kernel/irq_manager.hpp>

bool OS::power_   {true};
MHz  OS::cpu_mhz_ {1000};

// Set default rsprint_handler
OS::rsprint_func OS::rsprint_handler_ = &OS::default_rsprint;
hw::Serial& OS::com1 = hw::Serial::port<1>();

extern "C" uint16_t _cpu_sampling_freq_divider_;

void OS::start() {

  // Initialize serial port
  com1.init();

  // Print a fancy header
  FILLINE('=');
  CAPTION("#include<os> // Literally\n");
  FILLINE('=');

  debug("\t[*] OS class started\n");
  srand(time(NULL));

  // Heap
  extern caddr_t heap_end;
  extern char    _end;

  MYINFO("Heap start: @ %p", heap_end);
  MYINFO("Current end is: @ %p", &_end);

  atexit(default_exit);

  // read ACPI tables
  hw::ACPI::init();

  // setup APIC, APIC timer, SMP etc.
  hw::APIC::init();

  // Set up interrupt handlers
  IRQ_manager::init();

  INFO("BSP", "Enabling interrupts");
  hw::APIC::setup_subs();
  IRQ_manager::enable_interrupts();

  // Initialize the Interval Timer
  hw::PIT::init();

  // Initialize PCI devices
  PCI_manager::init();

  // Estimate CPU frequency
  MYINFO("Estimating CPU-frequency");
  INFO2("|");
  INFO2("+--(10 samples, %f sec. interval)",
  (hw::PIT::frequency() / _cpu_sampling_freq_divider_).count());
  INFO2("|");

  // TODO: Debug why actual measurments sometimes causes problems. Issue #246.
  cpu_mhz_ = hw::PIT::CPUFrequency();

  INFO2("+--> %f MHz", cpu_mhz_.count());

  // Everything is ready
  MYINFO("Starting %s", Service::name().c_str());
  FILLINE('=');
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
    IRQ_manager::cpu(0).notify();
    debug("<OS> Woke up @ t = %li\n", uptime());
  }

  //Cleanup
  //Service::stop();
}

size_t OS::rsprint(const char* str) {
  size_t len = 0;

  // Measure length
  while (str[len++]);

  // Output callback
  rsprint_handler_(str, len);
  return len;
}

size_t OS::rsprint(const char* str, const size_t len) {
  // Output callback
  OS::rsprint_handler_(str, len);
  return len;
}

void OS::default_rsprint(const char* str, size_t len) {
  for(size_t i = 0; i < len; ++i)
    com1.write(str[i]);
}
