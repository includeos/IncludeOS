#include <os>
#include "../x86_pc/acpi.hpp"
#include "../x86_pc/pit.hpp"
#include "../x86_pc/apic.hpp"
#include "../x86_pc/apic_timer.hpp"
#include "../x86_pc/ioapic.hpp"
#include <kernel/os.hpp>

using namespace x86;
using namespace std::chrono;

extern "C" uint16_t _cpu_sampling_freq_divider_;

void __arch_poweroff()
{
  __asm__ __volatile__("cli; hlt");
  for(;;);
}

void __platform_init(){
  // FIXME: set up minimal CPU exception handlers
  // TODO: set up minimal CPU exception handlers
}

void __arch_reboot(){}
void __arch_enable_legacy_irq(unsigned char){}
void __arch_disable_legacy_irq(unsigned char){}


void SMP::global_lock() noexcept {}
void SMP::global_unlock() noexcept {}
int SMP::cpu_id() noexcept { return 0; }
int SMP::cpu_count() noexcept { return 1; }
