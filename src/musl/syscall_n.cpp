#include "stub.hpp"

long syscall(long /*number*/)
{
  return -ENOSYS;
}

extern "C"
long syscall_n(long i) {
  return stubtrace(syscall, "syscall", i);
}
