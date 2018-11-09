#include "common.hpp"
#include <stdio.h>

static long sys_rename(const char* /*oldpath*/, const char* /*newpath*/)
{
  // currently makes no sense, especially since we're read-only
  return -EROFS;
}

extern "C"
long syscall_SYS_rename(const char* oldpath, const char* newpath)
{
  return strace(sys_rename, "rename", oldpath, newpath);
}
