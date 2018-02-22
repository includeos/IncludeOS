#include "common.hpp"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <fs/vfs.hpp>

static inline unsigned round_up(unsigned n, unsigned div) {
  Expects(div > 0);
  return (n + div - 1) / div;
}

static int sys_stat(const char *path, struct stat *buf) {
  if (buf == nullptr)
  {
    //PRINT("stat(%s, %p) == %d\n", path, buf, -1);
    errno = EFAULT;
    return -1;
  }
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
      buf->st_blocks = buf->st_size > 0 ? round_up(buf->st_size, 512) : 0;
      buf->st_blksize = ent.fs().block_size();
      //PRINT("stat(%s, %p) == %d\n", path, buf, 0);
      return 0;
    }
    else {
      //PRINT("stat(%s, %p) == %d\n", path, buf, -1);
      errno = EIO;
      return -1;
    }
  }
  catch (...)
  {
    //PRINT("stat(%s, %p) == %d\n", path, buf, -1);
    errno = EIO;
    return -1;
  }
}

extern "C"
int syscall_SYS_stat(const char *pathname, struct stat *statbuf) {
  return strace(sys_stat, "stat", pathname, statbuf);
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
