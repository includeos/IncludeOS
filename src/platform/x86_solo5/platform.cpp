#include <os.hpp>
#include <smp>
extern "C" {
#include <solo5/solo5.h>
}

void __arch_poweroff()
{
  asm volatile("cli; hlt");
  for(;;);
}

// returns wall clock time in nanoseconds since the UNIX epoch
uint64_t __arch_system_time() noexcept
{
  return solo5_clock_wall();
}
timespec __arch_wall_clock() noexcept
{
  const uint64_t stamp = solo5_clock_wall();
  timespec result;
  result.tv_sec = stamp / 1000000000ul;
  result.tv_nsec = stamp % 1000000000ul;
  return result;
}

uint32_t __arch_rand32()
{
  // TODO: fix me!
  // NOTE: solo5 does not virtualize TSC
  return solo5_clock_wall() & 0xFFFFFFFF;
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
