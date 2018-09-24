#include "common.hpp"
#include <unistd.h>

static long sys_unlink(const char* /*pathname*/)
{
  /* technically path needs to be verified first */
  return -EROFS;
}

extern "C"
long syscall_SYS_unlink(const char *pathname)
{
  return strace(sys_unlink, "rmdir", pathname);
}
