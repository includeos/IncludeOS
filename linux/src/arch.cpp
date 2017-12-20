#include <cstdint>
#include <arch.hpp>
#include <kernel/os.hpp>

void* _ELF_START_;
void* _ELF_END_;

uintptr_t OS::heap_max() noexcept
{
  return (uintptr_t) -1;
}

bool OS::is_panicking() noexcept
{
  return false;
}

void __arch_subscribe_irq(uint8_t) {}

int64_t __arch_time_now() noexcept {
  return time(0);
}

void __arch_reboot()
{
  exit(0);
}
