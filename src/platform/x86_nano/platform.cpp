#include <os>
#include <kernel.hpp>
#include <kernel/rng.hpp>
#include <kprint>
#include "../x86_pc/idt.hpp"
#include <smp>

extern "C" void noop_eoi() {}
extern "C" void cpu_sampling_irq_handler() {}
extern "C" void blocking_cycle_irq_handler() {}
void (*current_eoi_mechanism)() = noop_eoi;
void (*current_intr_handler)() = nullptr;

void __arch_poweroff()
{
  while (1) asm("cli; hlt;");
  __builtin_unreachable();
}

void RNG::init()
{
  // the nano platform does not do anything extra
}

static void platform_init()
{
  // setup CPU exception handlers
  x86::idt_initialize_for_cpu(0);
}

void kernel::start(uint32_t boot_magic, uint32_t boot_addr)
{
  assert(boot_magic == MULTIBOOT_BOOTLOADER_MAGIC);
  kernel::multiboot(boot_addr);
  assert(kernel::memory_end() != 0);

  platform_init();
}

// not supported!
uint64_t __arch_system_time() noexcept {
  return 0;
}
timespec __arch_wall_clock() noexcept {
  return {0, 0};
}
// not supported!
void os::block() noexcept {}

// default to serial
void kernel::default_stdout(const char* str, const size_t len)
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

void os::halt() noexcept {
  asm("hlt");
}

// default stdout/logging states
__attribute__((weak))
bool os_enable_boot_logging = false;
__attribute__((weak))
bool os_default_stdout = false;
