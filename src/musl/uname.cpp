#include "common.hpp"
#include <sys/utsname.h>
#include <os.hpp>

static long sys_uname(struct utsname *buf) {
  if(UNLIKELY(buf == nullptr))
    return -EFAULT;

  strcpy(buf->sysname, "IncludeOS");

  strcpy(buf->nodename, "IncludeOS-node");

  strcpy(buf->release, os::version());

  strcpy(buf->version, os::version());

  strcpy(buf->machine, os::arch());

  return 0;
}

extern "C"
long syscall_SYS_uname(struct utsname *buf) {
  return strace(sys_uname, "uname", buf);
}
