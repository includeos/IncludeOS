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

void __arch_system_deactivate()
{
  // nada
}

#include <execinfo.h>
void print_backtrace()
{
  static const int NUM_ADDRS = 64;
  void*  addresses[NUM_ADDRS];

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
extern "C"
void panic(const char* why)
{
  printf("!! PANIC !!\nReason: %s\n", why);
  print_backtrace();
  raise(SIGINT);
  exit(1);
}

extern "C"
void __os_store_soft_reset(const void*, size_t) {
  // don't need to on this platform
}
