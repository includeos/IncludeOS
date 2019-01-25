#include <kernel/os.hpp>
#include <smp>

void __arch_poweroff()
{
  while (true) asm ("cli; hlt");
}

void __platform_init() {
  // minimal CPU exception handlers already set by solo5 tender
}

void __arch_reboot() {}
void __arch_enable_legacy_irq(unsigned char) {}
void __arch_disable_legacy_irq(unsigned char) {}
void __arch_subscribe_irq(unsigned char) {}

void SMP::global_lock() noexcept {}
void SMP::global_unlock() noexcept {}
int SMP::cpu_id() noexcept { return 0; }
int SMP::cpu_count() noexcept { return 1; }
void SMP::signal(int) { }
void SMP::add_task(SMP::task_func, int) { };
