#include <arch.hpp>
#include <cstdint>
#include <ctime>
#include <kernel/os.hpp>
# define weak_alias(name, aliasname) \
  extern __typeof (name) aliasname __attribute__ ((weak, alias (#name)));

typedef void (*ctor_t) ();
extern "C" __attribute__(( visibility("hidden") )) void default_ctor() {}
ctor_t __plugin_ctors_start = default_ctor;
weak_alias(__plugin_ctors_start, __plugin_ctors_end);
ctor_t __service_ctors_start = default_ctor;
weak_alias(__service_ctors_start, __service_ctors_end);

char _ELF_START_;
char _ELF_END_;

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
  clock_gettime(CLOCK_MONOTONIC, &tv);
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
