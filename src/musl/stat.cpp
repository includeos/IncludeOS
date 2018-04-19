#include "common.hpp"
#include <sys/stat.h>

#include <fs/vfs.hpp>
#include <util/bitops.hpp> // roundto

long sys_stat(const char *path, struct stat *buf)
{
  if (UNLIKELY(buf == nullptr))
    return -EFAULT;

  if (UNLIKELY(path == nullptr))
    return -ENOENT;

  memset(buf, 0, sizeof(struct stat));
  try {
    auto ent = fs::VFS::stat_sync(path);
    if (ent.is_valid())
    {
      if (ent.is_file()) buf->st_mode = S_IFREG;
      if (ent.is_dir()) buf->st_mode = S_IFDIR;
      buf->st_dev = ent.device_id();
      buf->st_ino = ent.block();
      buf->st_nlink = 1;
      buf->st_size = ent.size();
      buf->st_atime = ent.modified();
      buf->st_ctime = ent.modified();
      buf->st_mtime = ent.modified();
      buf->st_blocks = buf->st_size > 0 ? util::bits::roundto<512>(buf->st_size) : 0;
      buf->st_blksize = ent.fs().block_size();
      //printf("Is file? %s\n", S_ISREG(buf->st_mode) ? "YES" : "NO");
      //PRINT("stat(%s, %p) == %d\n", path, buf, 0);
      return 0;
    }
    else {
      //PRINT("stat(%s, %p) == %d\n", path, buf, -1);
      return -EIO;
    }
  }
  catch (...)
  {
    //PRINT("stat(%s, %p) == %d\n", path, buf, -1);
    return -EIO;
  }
}

static long sys_lstat(const char *path, struct stat *buf)
{
  // NOTE: should stat symlinks, instead of following them
  return sys_stat(path, buf);
}

extern "C"
long syscall_SYS_stat(const char *path, struct stat *buf) {
  return strace(sys_stat, "stat", path, buf);
}

extern "C"
long syscall_SYS_lstat(const char *path, struct stat *buf) {
  return strace(sys_lstat, "lstat", path, buf);
}
