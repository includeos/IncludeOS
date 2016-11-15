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

#include <service>
#include <memdisk>
#define __SPU__
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <ftw.h>

void print_stat(struct stat buffer);
int display_info(const char *fpath, const struct stat *sb, int flag, struct FTW *ftwbuf);
int add_filesize(const char *fpath, const struct stat *sb, int flag, struct FTW *ftwbuf);

static size_t total_size = 0;

int main()
{
  int res;
  char* nullbuf = nullptr;
  char shortbuf[4];
  char buf[1024];
  struct stat buffer;

  INFO("POSIX stat", "Running tests for POSIX stat");

  printf("nftw /\n");
  res = nftw("/", display_info, 20, FTW_PHYS | FTW_DEPTH);
  printf("nftw result: %d\n", res);
  if (res == -1)
  {
    printf("nftw error: %s\n", strerror(errno));
  }
  printf("Total size: %ld\n", total_size);

  res = stat("FOLDER1", nullptr);
  printf("stat("") with nullptr result: %d\n", res);
  if (res == -1)
  {
    printf("stat error: %s\n", strerror(errno));
  }
  else {
    print_stat(buffer);
  }
  CHECKSERT(res == -1 && errno == EFAULT, "stat() with nullptr buffer fails with EFAULT");

  res = stat("FOLDER1", &buffer);
  printf("stat(\"FOLDER1\") result: %d\n", res);
  if (res == -1)
  {
    printf("stat error: %s\n", strerror(errno));
  }
  else {
    print_stat(buffer);
  }
  CHECKSERT(res == 0, "stat() of folder that exists is ok");

  res = stat("FILE1", &buffer);
  printf("stat(\"FILE1\") result: %d\n", res);
  if (res == -1)
  {
    printf("stat error: %s\n", strerror(errno));
  }
  else {
    print_stat(buffer);
  }
  CHECKSERT(res == 0, "stat() of file that exists is ok");

  res = stat("FOLDER666", &buffer);
  printf("stat(\"FOLDER1\") result: %d\n", res);
  if (res == -1)
  {
    printf("stat error: %s\n", strerror(errno));
  }
  else {
    print_stat(buffer);
  }
  CHECKSERT(res == -1, "stat() of folder that does not exist fails");

  res = stat("FILE666", &buffer);
  printf("stat(\"FILE666\") result: %d\n", res);
  if (res == -1)
  {
    printf("stat error: %s\n", strerror(errno));
  }
  else {
    print_stat(buffer);
  }
  CHECKSERT(res == -1, "stat() of file that does not exist fails");

  res = chdir(nullptr);
  printf("chdir result (to nullptr): %d\n", res);
  if (res == -1)
  {
    printf("chdir error: %s\n", strerror(errno));
  }
  CHECKSERT(res == -1, "chdir(nullptr) should fail");

  res = chdir("");
  printf("chdir result (to empty string): %d\n", res);
  if (res == -1)
  {
    printf("chdir error: %s\n", strerror(errno));
  }
  CHECKSERT(res == -1, "chdir(\"\") should fail");

  res = chdir("FILE2");
  printf("chdir result (not a folder): %d\n", res);
  if (res == -1)
  {
    printf("chdir error: %s\n", strerror(errno));
  }
  CHECKSERT(res == -1, "chdir() to a file should fail");

  res = chdir("FOLDER1");
  printf("chdir result (existing folder): %d\n", res);
  if (res == -1)
  {
    printf("chdir error: %s\n", strerror(errno));
  }
  CHECKSERT(res == 0, "chdir to folder that exists is ok");

  res = chdir("/FOLDER1");
  printf("chdir result (existing folder, absolute): %d\n", res);
  if (res == -1)
  {
    printf("chdir error: %s\n", strerror(errno));
  }

  res = chdir(".");
  printf("chdir result (to \".\"): %d\n", res);
  if (res == -1)
  {
    printf("chdir error: %s\n", strerror(errno));
  }
  CHECKSERT(res == 0, "chdir(\".\") is ok");

  res = chdir("FOLDERA");
  printf("chdir result (to subfolder of cwd): %d\n", res);
  if (res == -1)
  {
    printf("chdir error: %s\n", strerror(errno));
  }
  CHECKSERT(res == 0, "chdir to subfolder of cwd is ok");

  char* nullcwd = getcwd(nullbuf, 0);
  printf("getcwd result (nullptr, size 0): %s\n", nullcwd == nullptr ? "NULL" : nullcwd);
  if (nullcwd == nullptr)
  {
    printf("getcwd error: %s\n", strerror(errno));
  }
  CHECKSERT(nullcwd == nullptr && errno == EINVAL, "getcwd() with 0-size buffer should fail with EINVAL");

  nullcwd = getcwd(nullptr, 1024);
  printf("getcwd result (nullptr): %s\n", nullcwd == nullptr ? "NULL" : nullcwd);
  if (nullcwd == nullptr)
  {
    printf("getcwd error: %s\n", strerror(errno));
  }
  CHECKSERT(nullcwd == nullptr, "getcwd() with nullptr buffer should fail");

  char* shortcwd = getcwd(shortbuf, 4);
  printf("getcwd result (small buffer): %s\n", shortcwd == nullptr ? "NULL" : shortcwd);
  if (shortcwd == nullptr)
  {
    printf("getcwd error: %s\n", strerror(errno));
  }
  CHECKSERT(shortcwd == nullptr && errno == ERANGE, "getcwd() with too small buffer should fail with ERANGE");

  char* cwd = getcwd(buf, 1024);
  printf("getcwd result (adequate buffer): %s\n", cwd);
  if (cwd == nullptr)
  {
    printf("getcwd error: %s\n", strerror(errno));
  }
  CHECKSERT(cwd, "getcwd() with adequate buffer is ok");

  res = chmod("/dev/null", S_IRUSR);
  printf("chmod result: %d\n", res);
  if (res == -1)
  {
    printf("chmod error: %s\n", strerror(errno));
  }
  CHECKSERT(res == -1, "chmod() should fail on read-only memdisk");

  int fd = STDOUT_FILENO;
  close(fd);

  res = fchmod(fd, S_IWUSR);
  printf("fchmod result: %d\n", res);
  if (res == -1)
  {
    printf("fchmod error: %s\n", strerror(errno));
  }
  CHECKSERT(res == -1, "fchmod() on non-open FD should fail");

  res = fchmodat(fd, "test", S_IRUSR, AT_SYMLINK_NOFOLLOW);
  printf("fchmodat result: %d\n", res);
  if (res == -1)
  {
    printf("fchmodat error: %s\n", strerror(errno));
  }
  CHECKSERT(res == -1, "fchmodat() on non-open FD should fail");

  res = fstat(fd, &buffer);
  printf("fstat result: %d\n", res);
  if (res == -1)
  {
    printf("fstat error: %s\n", strerror(errno));
  }

  res = fstatat(fd, "test", &buffer, AT_SYMLINK_NOFOLLOW);
  printf("fstatat result: %d\n", res);
  if (res == -1)
  {
    printf("fstatat error: %s\n", strerror(errno));
  }
  CHECKSERT(res == -1, "fstatat() on non-open FD should fail");

  res = futimens(fd, nullptr);
  printf("futimens result: %d\n", res);
  if (res == -1)
  {
    printf("futimens error: %s\n", strerror(errno));
  }
  CHECKSERT(res == -1, "futimens() on non-open FD should fail");

  res = utimensat(fd, "test", nullptr, AT_SYMLINK_NOFOLLOW);
  printf("utimensat result: %d\n", res);
  if (res == -1)
  {
    printf("utimensat error: %s\n", strerror(errno));
  }
  CHECKSERT(res == -1, "utimensat() on non-open FD should fail");

/*
  res = lstat("/", &buffer);
  printf("lstat result: %d\n", res);
  if (res == -1)
  {
    printf("lstat error: %s\n", strerror(errno));
  }
*/

  res = mkdir("/dev/sda1/root",  S_IWUSR);
  printf("mkdir result: %d\n", res);
  if (res == -1)
  {
    printf("mkdir error: %s\n", strerror(errno));
  }
  CHECKSERT(res == -1, "mkdir() on read-only memdisk should fail");

  res = mkdirat(fd, "root",  S_IWUSR);
  printf("mkdirat result: %d\n", res);
  if (res == -1)
  {
    printf("mkdirat error: %s\n", strerror(errno));
  }
  CHECKSERT(res == -1, "mkdirat() on non-open FD should fail");

  res = mkfifo("/FILE_FIFO",  S_IWUSR);
  printf("mkfifo result: %d\n", res);
  if (res == -1)
  {
    printf("mkfifo error: %s\n", strerror(errno));
  }
  CHECKSERT(res == -1, "mkfifo() on read-only memdisk should fail");

  res = mkfifoat(AT_FDCWD, "test",  S_IWUSR);
  printf("mkfifoat result: %d\n", res);
  if (res == -1)
  {
    printf("mkfifoat error: %s\n", strerror(errno));
  }
  CHECKSERT(res == -1, "mkfifoat() on non-open FD should fail");

/*
  res = mknod("/dev/null",  S_IWUSR, 0);
  printf("mknod result: %d\n", res);
  if (res == -1) {
    printf("mknod error: %s\n", strerror(errno));
  }
*/

  res = mknodat(AT_FDCWD, "test",  S_IWUSR, 0);
  printf("mknodat result: %d\n", res);
  if (res == -1) {
    printf("mknodat error: %s\n", strerror(errno));
  }
  CHECKSERT(res == -1, "mknodat() on non-open FD should fail");

  mode_t old_umask = umask(0);
  printf("Old umask: %d\n", old_umask);



  res = nftw("MISSING_FILE", display_info, 20, FTW_PHYS);
  printf("nftw result: %d\n", res);
  if (res == -1)
  {
    printf("nftw error: %s\n", strerror(errno));
  }

  res = nftw("FILE1", display_info, 20, FTW_PHYS);
  printf("nftw result: %d\n", res);
  if (res == -1)
  {
    printf("nftw error: %s\n", strerror(errno));
  }

  printf("nftw /FOLDER1\n");
  res = nftw("/FOLDER1", display_info, 20, FTW_PHYS);
  printf("nftw result: %d\n", res);
  if (res == -1)
  {
    printf("nftw error: %s\n", strerror(errno));
  }

  printf("nftw /FOLDER2\n");
  res = nftw("/FOLDER2", display_info, 20, FTW_PHYS);
  printf("nftw result: %d\n", res);
  if (res == -1)
  {
    printf("nftw error: %s\n", strerror(errno));
  }

  printf("nftw /FOLDER3\n");
  res = nftw("/FOLDER3", display_info, 20, FTW_PHYS);
  printf("nftw result: %d\n", res);
  if (res == -1)
  {
    printf("nftw error: %s\n", strerror(errno));
  }

  printf("nftw /\n");
  res = nftw("/", display_info, 20, FTW_PHYS);
  printf("nftw result: %d\n", res);
  if (res == -1)
  {
    printf("nftw error: %s\n", strerror(errno));
  }

  INFO("POSIX STAT", "All done!");
  exit(0);
}

void print_stat(struct stat buffer)
{
    printf("st_dev: %d\n", buffer.st_dev);
    printf("st_ino: %hu\n", buffer.st_ino);
    printf("st_mode: %d\n", buffer.st_mode);
    printf("st_nlink: %d\n", buffer.st_nlink);
    printf("st_uid %d\n", buffer.st_uid);
    printf("st_gid: %d\n", buffer.st_gid);
    printf("st_rdev: %d\n", buffer.st_rdev);
    printf("st_size: %ld\n", buffer.st_size);
    printf("st_atime: %ld\n", buffer.st_atime);
    printf("st_ctime: %ld\n", buffer.st_ctime);
    printf("st_mtime: %ld\n", buffer.st_mtime);
    printf("st_blksize: %ld\n", buffer.st_blksize);
    printf("st_blocks: %ld\n", buffer.st_blocks);
}

int display_info(const char *fpath, const struct stat *sb, int flag, struct FTW *ftwbuf)
{
  printf("%ld\t%s (%d)\n", sb->st_size, fpath, flag);
  return 0;
}

int add_filesize(const char *fpath, const struct stat *sb, int flag, struct FTW *ftwbuf)
{
  total_size += sb->st_size;
  return 0;
}
