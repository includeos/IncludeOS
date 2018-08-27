#include "common.hpp"

static long sys_set_robust_list(struct robust_list_head */*head*/, size_t /*len*/)
{
  return -ENOSYS;
}

extern "C"
long syscall_SYS_set_robust_list(struct robust_list_head *head, size_t len) {
  return strace(sys_set_robust_list, "set_robust_list", head, len);
}
