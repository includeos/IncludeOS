#include "common.hpp"
#include <sys/sysinfo.h>

static long sys_sysinfo(struct sysinfo */*info*/)
{
  return -ENOSYS;
}

extern "C"
long syscall_SYS_sysinfo(struct sysinfo *info) {
  return strace(sys_sysinfo, "sysinfo", info);
}
