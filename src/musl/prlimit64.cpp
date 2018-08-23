#include "stub.hpp"

static long sys_prlimit64()
{
  return 0;
}

extern "C"
long syscall_SYS_prlimit64() {
  return stubtrace(sys_prlimit64, "prlimit64");
}
