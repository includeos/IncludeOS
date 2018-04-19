#include "common.hpp"
#include <sys/stat.h>

mode_t THE_MASK = 002;

static mode_t sys_umask(mode_t cmask) {
  mode_t prev_mask = THE_MASK;

  if(THE_MASK != cmask)
    THE_MASK = cmask;

  return prev_mask;
}

extern "C"
mode_t syscall_SYS_umask(mode_t cmask) {
  return strace(sys_umask, "umask", cmask);
}
