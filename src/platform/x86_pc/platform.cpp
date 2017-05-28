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
#include <hw/devices.hpp>
#include <info>
#define MYINFO(X,...) INFO("x86", X, ##__VA_ARGS__)

extern "C" uint16_t _cpu_sampling_freq_divider_;
extern "C" void* get_cpu_esp();
extern "C" void* get_cpu_ebp();

namespace tls {
  size_t get_tls_size();
  void   fill_tls_data(char*);
}
struct smp_table
{
  // thread self-pointer
  smp_table* tls_data;
  // per-cpu data
  int cpuid;
  int unused;
};

using namespace x86;

void __platform_init()
{
  // read ACPI tables
  ACPI::init();

  // setup APIC, APIC timer, SMP etc.
  APIC::init();

  // enable fs/gs for local APIC
  initialize_gdt_for_cpu(APIC::get().get_id());

  // IDT manager: Interrupt and exception handlers
  IRQ_manager::init();

  // initialize and start registered APs found in ACPI-tables
#ifndef INCLUDEOS_SINGLE_THREADED
  x86::init_SMP();
#endif

  // enable interrupts
  MYINFO("Enabling interrupts");
  IRQ_manager::enable_interrupts();

  // Estimate CPU frequency
  MYINFO("Estimating CPU-frequency");
  INFO2("|");
  INFO2("+--(%d samples, %f sec. interval)", 18,
        (x86::PIT::FREQUENCY / _cpu_sampling_freq_divider_).count());
  INFO2("|");

  if (OS::cpu_freq().count() <= 0.0) {
    OS::cpu_mhz_ = MHz(PIT::get().estimate_CPU_frequency());
  }
  INFO2("+--> %f MHz", OS::cpu_freq().count());

  // Note: CPU freq must be known before we can start timer system
  // Initialize APIC timers and timer systems
  // Deferred call to Service::ready() when calibration is complete
  APIC_Timer::calibrate();

  // Initialize PCI devices
  PCI_manager::init();

  // Print registered devices
  hw::Devices::print_devices();
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

#include <malloc.h>
namespace x86
{
  struct alignas(SMP_ALIGN) segtable
  {
    int cpuid;
    struct GDT gdt;
    char  stuff[128];
    void* selfptr;
  };
  static std::array<segtable, SMP_MAX_CORES> gdtables;
#ifdef ARCH_i686
  struct alignas(SMP_ALIGN) tls_table
  {
    char* tls_data;
  };
  static std::array<tls_table, SMP_MAX_CORES> tlstables;
#endif

  void initialize_gdt_for_cpu(int id)
  {
#ifdef ARCH_x86_64
    size_t thread_size = tls::get_tls_size();
    size_t total_size  = thread_size + sizeof(smp_table);
    char* data = (char*) memalign(64, total_size);
    tls::fill_tls_data(&data[thread_size]);

    auto* table = (smp_table*) &data[thread_size];
    table->tls_data = table;
    table->cpuid    = id;
    GDT::set_gs(&table->cpuid); // PER_CPU on gs
    GDT::set_fs(table); // TLS self-ptr in fs

#else
    tlstables[id].tls_data = tls::get_tls_data(id);
    gdtables[id].cpuid = id;
    // initialize GDT for this core
    auto& gdt = gdtables.at(id).gdt;
    gdt.initialize();
    // create per-thread segment
    int gs = gdt.create_data(&tlstables[id], 1);
    // create PER-CPU segment
    int fs = gdt.create_data(&gdtables[id], 1);
    // load GDT and refresh segments
    GDT::reload_gdt(gdt);
    // enable per-cpu and per-thread
    GDT::set_fs(fs);
    GDT::set_gs(gs);
#endif
  }
} // x86
