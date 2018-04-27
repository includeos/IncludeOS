#include "common.hpp"

extern "C"
long syscall_SYS__lseek() {
  STUB("_lseek");
  return 0;
}
