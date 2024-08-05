#include "stub.hpp"

static long sys_gettid() {
// TODO:  threading partially implemented. Needs cleaning up.
// We used to warn:
// "gettid not implemented for threaded IncludeOS"
// But we want to enable -Werror
  return 1;
}

extern "C"
long syscall_SYS_gettid() {
  return stubtrace(sys_gettid, "gettid");
}
