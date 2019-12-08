
#include "acpi.hpp"
#include "apic.hpp"
#include "apic_timer.hpp"
#include "clocks.hpp"
#include "idt.hpp"
#include "smbios.hpp"
#include "smp.hpp"
#include <arch/x86/gdt.hpp>
#include <kernel/events.hpp>
#include <kernel/threads.hpp>
#include <hw/pci_manager.hpp>
#include <kernel.hpp>
#include <os.hpp>
#include <info>
//#define ENABLE_PROFILERS
#include <profile>
#define MYINFO(X,...) INFO("x86", X, ##__VA_ARGS__)

extern "C" char* get_cpu_esp();
extern "C" void* get_cpu_ebp();

struct alignas(64) smp_table
{
  // per-cpu cpuid
  int cpuid;
  /** put more here **/
};
static std::vector<smp_table> cpu_tables;

namespace x86 {
  void initialize_cpu_tables_for_cpu(int cpu);
  void register_deactivation_function(delegate<void()>);
}
namespace kernel {
	Fixed_vector<delegate<void()>, 64> smp_global_init(Fixedvector_Init::UNINIT);
}

void __platform_init()
{
  // read ACPI tables
  {
    PROFILE("ACPI init");
    x86::ACPI::init();
  }

  // resize up all PER-CPU structures
  for (auto lambda : kernel::smp_global_init) { lambda(); }

  // setup main thread after PER-CPU ctors
  kernel::setup_main_thread(0);

  // read SMBIOS tables
  {
    PROFILE("SMBIOS init");
    x86::SMBIOS::init();
  }

  // enable fs/gs for local APIC
  INFO("x86", "Setting up GDT, TLS, IST");
  //initialize_gdt_for_cpu(0);
#ifdef ARCH_x86_64
  // setup Interrupt Stack Table
  {
    PROFILE("IST amd64");
    x86::ist_initialize_for_cpu(0, 0x9D3F0);
  }
#endif

  INFO("x86", "Initializing CPU 0");
  {
    PROFILE("CPU tables x86");
    x86::initialize_cpu_tables_for_cpu(0);
  }

  {
    PROFILE("Events init");
    Events::get(0).init_local();
  }

  // setup APIC, APIC timer, SMP etc.
  {
    PROFILE("APIC init");
    x86::APIC::init();
  }

  // enable interrupts
  MYINFO("Enabling interrupts");
  {
    PROFILE("Enable interrupts");
    asm volatile("sti");
  }

  // initialize and start registered APs found in ACPI-tables
  {
    PROFILE("SMP init");
    x86::init_SMP();
  }

  // Setup kernel clocks
  MYINFO("Setting up kernel clock sources");
  {
    PROFILE("Clocks init (x86)");
    x86::Clocks::init();
  }

  if (os::cpu_freq().count() <= 0.0) {
    kernel::state().cpu_khz = x86::Clocks::get_khz();
  }
  INFO2("+--> %f MHz", os::cpu_freq().count() / 1000.0);

  // Note: CPU freq must be known before we can start timer system
  // Initialize APIC timers and timer systems
  // Deferred call to Service::ready() when calibration is complete
  {
    PROFILE("APIC timer calibrate");
    x86::APIC_Timer::calibrate();
  }

  INFO2("Initializing drivers");
  {
    PROFILE("Initialize drivers");
    extern kernel::ctor_t __driver_ctors_start;
    extern kernel::ctor_t __driver_ctors_end;
    kernel::run_ctors(&__driver_ctors_start, &__driver_ctors_end);
  }

  // Scan PCI buses
  {
    PROFILE("PCI bus scan");
    hw::PCI_manager::init();
  }
  {
	PROFILE("PCI device init")
    // Initialize storage devices
    hw::PCI_manager::init_devices(PCI::STORAGE);
    kernel::state().block_drivers_ready = true;
    // Initialize network devices
    hw::PCI_manager::init_devices(PCI::NIC);
  }

  // Print registered devices
  os::machine().print_devices();
}

#ifdef ARCH_i686
static x86::GDT gdt;
#endif

void x86::initialize_cpu_tables_for_cpu(int cpu)
{
  if (cpu == 0) {
	  cpu_tables.resize(SMP::early_cpu_total());
  }
  cpu_tables[cpu].cpuid = cpu;

#ifdef ARCH_x86_64
  x86::CPU::set_gs(&cpu_tables[cpu]);
#else
  int fs = gdt.create_data(&cpu_tables[cpu], 1);
  GDT::reload_gdt(gdt);
  GDT::set_fs(fs);
#endif
}

static std::vector<delegate<void()>> deactivate_funcs;
void x86::register_deactivation_function(delegate<void()> func) {
  deactivate_funcs.push_back(std::move(func));
}
void __arch_system_deactivate()
{
  for (auto& func : deactivate_funcs) func();
}

void __arch_enable_legacy_irq(uint8_t irq)
{
  x86::APIC::enable_irq(irq);
}
void __arch_disable_legacy_irq(uint8_t irq)
{
  x86::APIC::disable_irq(irq);
}

void __arch_poweroff()
{
  x86::ACPI::shutdown();
  __builtin_unreachable();
}
void __arch_reboot()
{
  x86::ACPI::reboot();
  __builtin_unreachable();
}
