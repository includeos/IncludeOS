#include "stub.hpp"
#include <sys/types.h>

static long sys_creat(const char* /*pathname*/, mode_t /*mode*/) {
  // currently makes no sense, especially since we're read-only
  return -EROFS;
}

extern "C"
long syscall_SYS_creat(const char *pathname, mode_t mode) {
  return stubtrace(sys_creat, "creat", pathname, mode);
}
