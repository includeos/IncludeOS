#include "stub.hpp"

static long sys_set_robust_list()
{
  return 0;
}

extern "C"
long syscall_SYS_set_robust_list() {
  return stubtrace(sys_set_robust_list, "set_robust_list");
}
