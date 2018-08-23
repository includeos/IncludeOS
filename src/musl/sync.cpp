#include "stub.hpp"

static long sys_sync() {
  return 0;
}

static long sys_syncfs() {
  return 0;
}

extern "C" {
long syscall_SYS_sync() {
  return stubtrace(sys_sync, "sync");
}

long syscall_SYS_syncfs() {
  return stubtrace(sys_syncfs, "syncfs");
}
}
