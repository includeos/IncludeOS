#include "common.hpp"

extern "C" {
long syscall_SYS_sync() {
  STUB("sync");
  return 0;
}

long syscall_SYS_syncfs() {
  STUB("syncfs");
  return 0;
}
}
