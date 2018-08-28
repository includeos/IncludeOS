#include "common.hpp"

#include <sys/resource.h>

static int sys_getrlimit(int /*resource*/, struct rlimit*) {
  return -ENOSYS;
}

extern "C"
long syscall_SYS_getrlimit(int resource, struct rlimit *rlim) {
  return strace(sys_getrlimit, "getrlimit", resource, rlim);
}
