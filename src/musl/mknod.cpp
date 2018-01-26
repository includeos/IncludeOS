#include "common.hpp"

extern "C" {
long syscall_SYS_mknod() {
  STUB("mknod");
  return 0;
}

long syscall_SYS_mknodat() {
  STUB("mknod");
  return 0;
}
}
