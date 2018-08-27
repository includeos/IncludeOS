#include "stub.hpp"

static long sys__lseek()
{
  return -ENOSYS;
}

extern "C"
long syscall_SYS__lseek() {
  return stubtrace(sys__lseek, "_lseek");
}
