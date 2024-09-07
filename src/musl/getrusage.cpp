#include "common.hpp"

#include <sys/resource.h>

static int sys_getrusage(int /*resource*/, struct rusage*) {
  return -ENOSYS;
}

extern "C"
long syscall_SYS_getrusage(int resource, struct rusage *usage) {
  return strace(sys_getrusage, "getrusage", resource, usage);
}
