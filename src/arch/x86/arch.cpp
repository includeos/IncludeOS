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

#include "acpi.hpp"
#include "apic.hpp"
#include "apic_timer.hpp"
#include "gdt.hpp"
#include "pit.hpp"
#include "smp.hpp"
#include <kernel/irq_manager.hpp>
#include <kernel/pci_manager.hpp>
#include <kernel/os.hpp>
#include <info>
#define MYINFO(X,...) INFO("x86", X, ##__VA_ARGS__)

extern "C" uint16_t _cpu_sampling_freq_divider_;

using namespace x86;

namespace x86 {
  static void initialize_cpu_shared();
}

void __arch_init()
{
  // read ACPI tables
  ACPI::init();
  // create CPU storage struct
  initialize_cpu_shared();

  // setup APIC, APIC timer, SMP etc.
  APIC::init();

  // set fs/gs for local APIC
  initialize_gdt_for_cpu(APIC::get().get_id());

  // IDT manager: Interrupt and exception handlers
  IRQ_manager::init(APIC::get().get_id());

  // initialize and start registered APs found in ACPI-tables
#ifndef INCLUDEOS_SINGLE_THREADED
  x86::SMP::init();
#endif

  // enable interrupts
  MYINFO("Enabling interrupts");
  IRQ_manager::enable_interrupts();

  // Initialize the Interval Timer
  PIT::init();

  // Initialize PCI devices
  PCI_manager::init();

  // Print registered devices
  hw::Devices::print_devices();

  // Estimate CPU frequency
  MYINFO("Estimating CPU-frequency");
  INFO2("|");
  INFO2("+--(%d samples, %f sec. interval)", 18,
        (x86::PIT::frequency() / _cpu_sampling_freq_divider_).count());
  INFO2("|");

  if (OS::cpu_freq().count() <= 0.0) {
    OS::cpu_mhz_ = MHz(PIT::estimate_CPU_frequency());
  }
  INFO2("+--> %f MHz", OS::cpu_freq().count());

  // Note: CPU freq must be known before we can start timer system
  // initialize BSP APIC timer
  // call Service::ready when calibrated
  APIC_Timer::calibrate();
}

void __arch_enable_legacy_irq(uint8_t irq)
{
  APIC::enable_irq(irq);
}
void __arch_disable_legacy_irq(uint8_t irq)
{
  APIC::disable_irq(irq);
}

void __arch_poweroff()
{
  ACPI::shutdown();
  __builtin_unreachable();
}
void __arch_reboot()
{
  ACPI::reboot();
  __builtin_unreachable();
}

namespace x86
{
  struct alignas(SMP_ALIGN) cpu_shared
  {
    int cpduid;
  };
  static std::array<cpu_shared, SMP_MAX_CORES> cpudata; // for alignment

  static void initialize_cpu_shared()
  {
    for (size_t id = 0; id < cpudata.size(); id++) {
      cpudata[id].cpduid = id;
    }
  }

  struct alignas(SMP_ALIGN) segtable
  {
    struct GDT gdt;
  };
  static std::array<segtable, SMP_MAX_CORES> gdtables;

  void initialize_gdt_for_cpu(int id)
  {
    // initialize GDT for this core
    gdtables.at(id).gdt.initialize();
    // create PER-CPU segment
    int fs = gdtables[id].gdt.create_data(&cpudata.at(id), 1);
    // load GDT and refresh segments
    GDT::reload_gdt(gdtables[id].gdt);
    // enable per-cpu for this core
    cpudata[id].cpduid = id;
    GDT::set_fs(fs);
  }
} // x86
