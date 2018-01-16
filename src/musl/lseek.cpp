#include "common.hpp"

extern "C"
long syscall_SYS_lseek() {
  STUB("lseek");
  return 0;
}
