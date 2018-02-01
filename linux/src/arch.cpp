#include <arch.hpp>
#include <cstdint>
#include <ctime>
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

uint64_t __arch_system_time() noexcept
{
  struct timespec tv;
  clock_gettime(CLOCK_REALTIME, &tv);
  return tv.tv_sec*(uint64_t)1000000000ull+tv.tv_nsec;
}
timespec __arch_wall_clock() noexcept
{
  struct timespec tv;
  clock_gettime(CLOCK_REALTIME, &tv);
  return tv;
}

void __arch_reboot()
{
  exit(0);
}
