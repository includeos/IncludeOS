#include "common.hpp"
#include <signal.h>

static long sys_sigaltstack(const stack_t * /*ss*/, stack_t * /*old_ss*/)
{
  return -ENOSYS;
}

extern "C"
long syscall_SYS_sigaltstack(const stack_t *_Nullable ss,
                                stack_t *_Nullable old_ss)
{
  return strace(sys_sigaltstack, "sigaltstack", ss, old_ss);
}
