#include "common.hpp"

#include <fs/vfs.hpp>

static long sys_access(const char *pathname, int mode) {
  if (UNLIKELY(pathname == nullptr))
    return -EFAULT;

  if (UNLIKELY(pathname[0] == 0))
    return -ENOENT;

  constexpr auto our_flags = R_OK | F_OK; // read only

  try
  {
    auto ent = fs::VFS::stat_sync(pathname);
    if (ent.is_valid()) {
      return ((mode & our_flags) == mode) ? 0 : -EROFS;
    }
    return -ENOENT;
  }
  catch(const fs::Err_not_found&)
  {
    return -ENOENT;
  }
}

extern "C"
long syscall_SYS_access(const char *pathname, int mode) {
  return strace(sys_access, "access", pathname, mode);
}
