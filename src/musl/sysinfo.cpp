#include "stub.hpp"

static long sys_sysinfo()
{
  return 0;
}

extern "C"
long syscall_SYS_sysinfo() {
  return stubtrace(sys_sysinfo, "sysinfo");
}
