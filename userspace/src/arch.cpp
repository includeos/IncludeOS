#include <arch.hpp>
#include <cstdint>
#include <ctime>
#include <os.hpp>
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

#include <random>
uint32_t __arch_rand32()
{
  static std::random_device rd;
  static std::mt19937_64 gen(rd());
  static std::uniform_int_distribution<uint32_t> dis;
  return dis(gen);
}

void __arch_reboot()
{
  exit(0);
}

void __arch_system_deactivate()
{
  // nada
}

#ifdef __linux__
#include <execinfo.h>
#endif
void os::print_backtrace() noexcept
{
  static const int NUM_ADDRS = 64;
  void*  addresses[NUM_ADDRS];

#ifdef __linux__
  int nptrs = backtrace(addresses, NUM_ADDRS);
  printf("backtrace() returned %d addresses\n", nptrs);

  /* The call backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)
     would produce similar output to the following: */

  char** strings = backtrace_symbols(addresses, nptrs);
  if (strings == NULL) {
    perror("backtrace_symbols");
    exit(EXIT_FAILURE);
  }

  for (int j = 0; j < nptrs; j++)
      printf("#%02d: %8p %s\n", j, addresses[j], strings[j]);

  free(strings);
#endif
}

// context buffer
static std::array<char, 512> context_buffer;
size_t get_crash_context_length()
{
  return context_buffer.size();
}
char*  get_crash_context_buffer()
{
  return context_buffer.data();
}

#include <signal.h>
namespace os
{
  void panic(const char* why) noexcept
  {
    printf("!! PANIC !!\nReason: %s\n", why);
    print_backtrace();
    raise(SIGINT);
    exit(1);
  }
}

extern "C"
void __os_store_soft_reset(const void*, size_t) {
  // don't need to on this platform
}
