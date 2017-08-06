// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <sys/stat.h>
#include <errno.h>
#include <cstring>
#include <fd_map.hpp>
#include <fs/vfs.hpp>
#include <memdisk>
#include <net/tcp/packet.hpp>
#include <fcntl.h>
#include <unistd.h>

#ifndef DEFAULT_UMASK
#define DEFAULT_UMASK 002
#endif

extern fs::Disk_ptr& fs_disk();

int chmod(const char *path, mode_t mode) {
  (void) path;
  (void) mode;
  errno = EROFS;
  return -1;
}

int fchmod(int fildes, mode_t mode)
{
  try {
    auto& fd = FD_map::_get(fildes);
    return fd.fchmod(mode);
  }
  catch(const FD_not_found&) {
    errno = EBADF;
    return -1;
  }
}

int fchmodat(int filedes, const char *path, mode_t mode, int flag)
{
  try {
    auto& fd = FD_map::_get(filedes);
    return fd.fchmodat(path, mode, flag);
  }
  catch(const FD_not_found&) {
    errno = EBADF;
    return -1;
  }
}

int fstatat(int filedes, const char *path, struct stat *buf, int flag)
{
  if (filedes == AT_FDCWD)
  {
    char cwd_buf[PATH_MAX];
    char abs_path[PATH_MAX];
    if (getcwd(cwd_buf, PATH_MAX)) {
      snprintf(abs_path, PATH_MAX, "%s/%s", cwd_buf, path);
    }
    return stat(abs_path, buf);
  }
  else
  {
    try {
      auto& fd = FD_map::_get(filedes);
      return fd.fstatat(path, buf, flag);
    }
    catch(const FD_not_found&) {
      errno = EBADF;
      return -1;
    }
  }
}

int futimens(int filedes, const struct timespec times[2])
{
  try {
    auto& fd = FD_map::_get(filedes);
    return fd.futimens(times);
  }
  catch(const FD_not_found&) {
    errno = EBADF;
    return -1;
  }
}

int utimensat(int filedes, const char *path, const struct timespec times[2], int flag)
{
  try {
    auto& fd = FD_map::_get(filedes);
    return fd.utimensat(path, times, flag);
  }
  catch(const FD_not_found&) {
    errno = EBADF;
    return -1;
  }
}

int mkdir(const char *path, mode_t mode)
{
  (void) path;
  (void) mode;
  errno = EROFS;
  return -1;
}

int mkdirat(int filedes, const char *path, mode_t mode)
{
  try {
    auto& fd = FD_map::_get(filedes);
    return fd.mkdirat(path, mode);
  }
  catch(const FD_not_found&) {
    errno = EBADF;
    return -1;
  }
}

int mkfifo(const char *path, mode_t mode)
{
  (void) path;
  (void) mode;
  errno = EROFS;
  return -1;
}

int mkfifoat(int filedes, const char *path, mode_t mode)
{
  try {
    auto& fd = FD_map::_get(filedes);
    return fd.mkfifoat(path, mode);
  }
  catch(const FD_not_found&) {
    errno = EBADF;
    return -1;
  }
}

int mknod(const char *path, mode_t mode, dev_t dev)
{
  (void) path;
  (void) mode;
  (void) dev;
  errno = EROFS;
  return -1;
}

int mknodat(int filedes, const char *path, mode_t mode, dev_t dev)
{
  try {
    auto& fd = FD_map::_get(filedes);
    return fd.mknodat(path, mode, dev);
  }
  catch(const FD_not_found&) {
    errno = EBADF;
    return -1;
  }
}

int stat(const char *path, struct stat *buf)
{
  if (buf == nullptr)
  {
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
      buf->st_blksize = fs::MemDisk::SECTOR_SIZE;
      return 0;
    }
    else {
      errno = EIO;
      return -1;
    }
  }
  catch (...)
  {
    printf("stat %s NOT FOUND\n", path);
    errno = EIO;
    return -1;
  }
}
int lstat(const char *path, struct stat *buf)
{
  // NOTE: should stat symlinks, instead of following them
  return stat(path, buf);
}

mode_t umask(mode_t cmask)
{
  (void) cmask;
  return DEFAULT_UMASK;
}

int fstat(int, struct stat* st) {
  debug("SYSCALL FSTAT Dummy, returning OK 0");
  st->st_mode = S_IFCHR;
  return 0;
}
