#include <sys/resource.h>

#include "stub.hpp"

static int sys_getrlimit(int resource, struct rlimit *rlim) {
  errno = ENOSYS;
  return -1;
}

extern "C"
long syscall_SYS_getrlimit(int resource, struct rlimit *rlim) {
  return stubtrace(sys_getrlimit, "getrlimit", resource, rlim);
}
