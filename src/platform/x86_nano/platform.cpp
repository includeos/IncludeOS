#include <os>
#include <kprint>
#include "../x86_pc/idt.hpp"

extern "C" void noop_eoi() {}
extern "C" void cpu_sampling_irq_handler() {}
void (*current_eoi_mechanism)() = noop_eoi;
void (*current_intr_handler)() = nullptr;

void __arch_poweroff()
{
  asm("cli; hlt;");
  __builtin_unreachable();
}

void __platform_init()
{
  // setup CPU exception handlers
  x86::idt_initialize_for_cpu(0);
}

// not supported!
int64_t __arch_time_now() noexcept {
  return 0;
}
// not supported!
void OS::block() {}
// default to serial
void OS::default_stdout(const char* str, const size_t len)
{
  __serial_print(str, len);
}

void __arch_reboot(){}
void __arch_enable_legacy_irq(unsigned char){}
void __arch_disable_legacy_irq(unsigned char){}


void SMP::global_lock() noexcept {}
void SMP::global_unlock() noexcept {}
int SMP::cpu_id() noexcept { return 0; }
int SMP::cpu_count() noexcept { return 1; }


// Support for the boot_logger plugin.
__attribute__((weak))
bool os_enable_boot_logging = false;
