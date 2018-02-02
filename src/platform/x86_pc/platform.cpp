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
#include "clocks.hpp"
#include "gdt.hpp"
#include "idt.hpp"
#include "smbios.hpp"
#include "smp.hpp"
#include <kernel/events.hpp>
#include <kernel/pci_manager.hpp>
#include <kernel/os.hpp>
#include <hw/devices.hpp>
#include <hw/pci_device.hpp>
#include <info>
#define MYINFO(X,...) INFO("x86", X, ##__VA_ARGS__)

extern "C" char* get_cpu_esp();
extern "C" void* get_cpu_ebp();
#define _SENTINEL_VALUE_   0x123456789ABCDEF

namespace tls {
  size_t get_tls_size();
  void   fill_tls_data(char*);
}
struct alignas(64) smp_table
{
  // thread self-pointer
  void* tls_data; // 0x0
  // per-cpu cpuid (and more)
  int cpuid;
  int reserved;

#ifdef ARCH_x86_64
  uintptr_t pad[3]; // 64-bit padding
  uintptr_t guard; // _SENTINEL_VALUE_
#else
  uint32_t  pad[2];
  uintptr_t guard; // _SENTINEL_VALUE_
  x86::GDT gdt; // 32-bit GDT
#endif
  /** put more here **/
};
#ifdef ARCH_x86_64
// FS:0x28 on Linux is storing a special sentinel stack-guard value
static_assert(offsetof(smp_table, guard) == 0x28, "Linux stack sentinel");
#endif

using namespace x86;
namespace x86 {
  void initialize_tls_for_smp();
}

void __platform_init()
{
  // read ACPI tables
  ACPI::init();

  // read SMBIOS tables
  SMBIOS::init();

  // setup process tables
  INFO("x86", "Setting up TLS");
  initialize_tls_for_smp();

  // enable fs/gs for local APIC
  INFO("x86", "Setting up GDT, TLS, IST");
  initialize_gdt_for_cpu(0);
#ifdef ARCH_x86_64
  // setup Interrupt Stack Table
  x86::ist_initialize_for_cpu(0, 0x9D3F0);
#endif

  // IDT manager: Interrupt and exception handlers
  INFO("x86", "Creating CPU exception handlers");
  x86::idt_initialize_for_cpu(0);
  Events::get(0).init_local();

  // setup APIC, APIC timer, SMP etc.
  APIC::init();

  // enable interrupts
  MYINFO("Enabling interrupts");
  asm volatile("sti");

  // initialize and start registered APs found in ACPI-tables
#ifndef INCLUDEOS_SINGLE_THREADED
  x86::init_SMP();
#endif

  // Setup kernel clocks
  MYINFO("Setting up kernel clock sources");
  Clocks::init();

  if (OS::cpu_freq().count() <= 0.0) {
    OS::cpu_khz_ = Clocks::get_khz();
  }
  INFO2("+--> %f MHz", OS::cpu_freq().count() / 1000.0);

  // Note: CPU freq must be known before we can start timer system
  // Initialize APIC timers and timer systems
  // Deferred call to Service::ready() when calibration is complete
  APIC_Timer::calibrate();

  // Initialize storage devices
  PCI_manager::init(PCI::STORAGE);
  OS::m_block_drivers_ready = true;
  // Initialize network devices
  PCI_manager::init(PCI::NIC);

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
  constexpr size_t smp_table_aligned_size()
  {
    size_t size = sizeof(smp_table);
    if (size & 63) size += 64 - (size & 63);
    return size;
  }

  static std::vector<char*> tls_buffers;

  void initialize_tls_for_smp()
  {
    const size_t thread_size = tls::get_tls_size();
    const size_t total_size  = thread_size + smp_table_aligned_size();
    const size_t cpu_count = ACPI::get_cpus().size();

    //printf("TLS buffers are %lu bytes, SMP table %lu bytes\n", total_size, smp_table_aligned_size());
    char* buffer = (char*) memalign(64, total_size * cpu_count);
    tls_buffers.reserve(cpu_count);
    for (auto cpu = 0u; cpu < cpu_count; cpu++)
        tls_buffers.push_back(&buffer[total_size * cpu]);
  }

  void initialize_gdt_for_cpu(int cpu_id)
  {
    char* tls_data  = tls_buffers.at(cpu_id);
    char* tls_table = tls_data + tls::get_tls_size();
    // TLS data at front of buffer
    tls::fill_tls_data(tls_data);
    // SMP control block after TLS data
    auto* table = (smp_table*) tls_table;
    table->tls_data = tls_data;
    table->cpuid    = cpu_id;
    table->guard    = (uintptr_t) _SENTINEL_VALUE_;
    // should be at least 8-byte aligned
    assert((((uintptr_t) table) & 7) == 0);
#ifdef ARCH_x86_64
    GDT::set_fs(table); // TLS self-ptr in fs
    GDT::set_gs(&table->cpuid); // PER_CPU on gs
#else
    // initialize GDT for this core
    auto& gdt = table->gdt;
    gdt.initialize();
    // create PER-CPU segment
    int fs = gdt.create_data(&table->cpuid, 1);
    // create per-thread segment
    int gs = gdt.create_data(table, 0xffffffff);
    // load GDT and refresh segments
    GDT::reload_gdt(gdt);
    // enable per-cpu and per-thread
    GDT::set_fs(fs);
    GDT::set_gs(gs);
#endif
    // hardware barrier
    __sync_synchronize();
  }
} // x86
