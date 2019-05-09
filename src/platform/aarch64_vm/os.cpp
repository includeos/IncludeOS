#include <kernel.hpp>
#include <timer.h>
#include <service>

#include <kernel/events.hpp>
#include "platform.hpp"

#define DEBUG
#define MYINFO(X,...) INFO("Kernel", X, ##__VA_ARGS__)

extern bool os_default_stdout;

struct alignas(SMP_ALIGN) OS_CPU {
  uint64_t cycles_hlt = 0;
};
static SMP::Array<OS_CPU> os_per_cpu;

uint64_t os::cycles_asleep() noexcept {
  return PER_CPU(os_per_cpu).cycles_hlt;
}
uint64_t os::nanos_asleep() noexcept {
  return (PER_CPU(os_per_cpu).cycles_hlt * 1e6) / os::cpu_freq().count();
}

extern kernel::ctor_t __stdout_ctors_start;
extern kernel::ctor_t __stdout_ctors_end;


void kernel::start(uint64_t fdt_addr) // boot_magic, uint32_t boot_addr)
{
  kernel::state().cmdline = Service::binary_name();

  // Initialize stdout handlers
  if(os_default_stdout) {
    os::add_stdout(&kernel::default_stdout);
  }

  kernel::run_ctors(&__stdout_ctors_start, &__stdout_ctors_end);

  // Print a fancy header
  CAPTION("#include<os> // Literally");
  __platform_init(fdt_addr);
}

void os::block() noexcept {


}

__attribute__((noinline))
void os::halt() noexcept{
  uint64_t cycles_before = os::Arch::cpu_cycles();
  asm volatile("wfi" :::"memory");
  //asm volatile("hlt #0xf000");
  asm volatile(
  ".global _irq_cb_return_location;\n"
  "_irq_cb_return_location:" );

  // Count sleep cycles
  PER_CPU(os_per_cpu).cycles_hlt += os::Arch::cpu_cycles() - cycles_before;

}

void os::event_loop()
{
  Events::get(0).process_events();
  do {
    os::halt();
    Events::get(0).process_events();
  } while (kernel::is_running());

  MYINFO("Stopping service");
  Service::stop();

  MYINFO("Powering off");
  __arch_poweroff();
}
