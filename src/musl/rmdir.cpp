#include "common.hpp"
#include <unistd.h>

static long sys_rmdir(const char* /*pathname*/)
{
  /* technically path needs to be verified first */
  return -EROFS;
}

extern "C"
long syscall_SYS_rmdir(const char *pathname)
{
  return strace(sys_rmdir, "rmdir", pathname);
}
