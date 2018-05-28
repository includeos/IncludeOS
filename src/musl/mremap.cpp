#include "common.hpp"

extern "C"
long syscall_SYS_mremap() {
  STUB("mremap");
  return -ENOSYS;
}
