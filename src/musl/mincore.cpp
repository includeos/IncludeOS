#include "stub.hpp"

static long sys_mincore()
{
  return 0;
}

extern "C"
long syscall_SYS_mincore()
{
  return stubtrace(sys_mincore, "mincore");
}
