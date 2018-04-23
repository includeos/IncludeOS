#include <sys/resource.h>

#include "stub.hpp"

static int sys_getrlimit(int /*resource*/, struct rlimit*) {
  return -ENOSYS;
}

extern "C"
long syscall_SYS_getrlimit(int resource, struct rlimit *rlim) {
  return stubtrace(sys_getrlimit, "getrlimit", resource, rlim);
}
