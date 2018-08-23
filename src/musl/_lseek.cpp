#include "stub.hpp"

static long sys__lseek()
{
  return 0;
}

extern "C"
long syscall_SYS__lseek() {
  return stubtrace(sys__lseek, "_lseek");
}
