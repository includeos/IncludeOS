#include "common.hpp"

extern "C"
long syscall_SYS_lseek() {
  STUB("lseek");
  return 0;
}

extern "C"
long syscall_SYS__llseek() {
  STUB("_lseek");
  return 0;
}
