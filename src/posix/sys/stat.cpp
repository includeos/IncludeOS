#include <sys/stat.h>
#include <errno.h>
#include <cstring>
#include <memdisk>

#ifndef DEFAULT_UMASK
#define DEFAULT_UMASK 002
#endif

extern fs::Disk_ptr& fs_disk();

int chmod(const char *path, mode_t mode) {
  errno = EROFS;
  return -1;
}

int fchmod(int fildes, mode_t mode)
{
  errno = EROFS;
  return -1;
}

int fchmodat(int fd, const char *path, mode_t mode, int flag)
{
  errno = EROFS;
  return -1;
}

int fstatat(int fd, const char *path, struct stat *buf, int flag)
{
  // todo: get real
  errno = EIO;
  return -1;
}

int futimens(int fd, const struct timespec times[2])
{
  errno = EROFS;
  return -1;
}

int utimensat(int fd, const char *path, const struct timespec times[2], int flag)
{
  errno = EROFS;
  return -1;
}

int mkdir(const char *path, mode_t mode)
{
  errno = EROFS;
  return -1;
}

int mkdirat(int fd, const char *path, mode_t mode)
{
  errno = EROFS;
  return -1;
}

int mkfifo(const char *path, mode_t mode)
{
  errno = EROFS;
  return -1;
}

int mkfifoat(int fd, const char *path, mode_t mode)
{
  errno = EROFS;
  return -1;
}

int mknod(const char *path, mode_t mode, dev_t dev)
{
  errno = EROFS;
  return -1;
}

int mknodat(int fd, const char *path, mode_t mode, dev_t dev)
{
  errno = EROFS;
  return -1;
}

int stat(const char *path, struct stat *buf)
{
  auto ent = fs_disk()->fs().stat(path);
  if (ent.is_valid())
  {
    memset(buf, 0, sizeof(struct stat));
    if (ent.is_file()) buf->st_mode = S_IFREG;
    if (ent.is_dir()) buf->st_mode = S_IFDIR;
    buf->st_size = ent.size();
    buf->st_mtime = ent.modified();
    buf->st_blksize = fs::MemDisk::SECTOR_SIZE;
    return 0;
  }
  else
  {
    errno = EIO;
    return -1;
  }
}

mode_t umask(mode_t cmask)
{
  return DEFAULT_UMASK;
}
