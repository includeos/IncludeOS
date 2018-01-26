#include "common.hpp"

extern "C"
long syscall_SYS_stat() {
  STUB("stat");
  return 0;
}

extern "C"
long syscall_SYS_lstat() {
  STUB("lstat");
  return 0;
}

extern "C"
long syscall_SYS_fstat() {
  STUB("fstat");
  return 0;
}

extern "C"
long syscall_SYS_fstatat() {
  STUB("fstatat");
  return 0;
}
