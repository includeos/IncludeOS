#include "stub.hpp"
#include <stdio.h>

static long sys_rename(const char* /*oldpath*/, const char* /*newpath*/)
{
  return -ENOSYS;
}

extern "C"
long syscall_SYS_rename(const char* oldpath, const char* newpath)
{
  return stubtrace(sys_rename, "rename", oldpath, newpath);
}
