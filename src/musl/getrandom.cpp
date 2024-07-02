#include "common.hpp"
#include <kernel/rng.hpp>

// TODO Flags are ignored.
static long sys_getrandom(void* buf, size_t len, unsigned int flags)
{
  rng_absorb(buf, len);
  return len;
}

extern "C"
long syscall_SYS_getrandom(void *buf, size_t len, unsigned int flags) {
  return strace(sys_getrandom, "getrandom", buf, len, flags);
}
