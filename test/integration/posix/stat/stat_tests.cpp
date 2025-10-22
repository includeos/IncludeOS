#include <service>
#include <info>
#include <cassert>
#define __SPU__
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

void print_stat(struct stat buffer);

void stat_tests()
{
  int res;
  char* nullbuf = nullptr;
  char shortbuf[4];
  char buf[1024];
  struct stat buffer;

  res = stat("folder1", nullptr);
  printf("stat("") with nullptr result: %d\n", res);
  if (res == -1)
  {
    printf("stat error: %s\n", strerror(errno));
  }
  else {
    print_stat(buffer);
  }
  CHECKSERT(res == -1 && errno == EFAULT, "stat() with nullptr buffer fails with EFAULT");

  res = stat("/mnt/disk/folder1", &buffer);
  printf("stat(\"folder1\") result: %d\n", res);
  if (res == -1)
  {
    printf("stat error: %s\n", strerror(errno));
  }
  else {
    print_stat(buffer);
  }
  CHECKSERT(res == 0, "stat() of folder that exists is ok");

  res = stat("/mnt/disk/file1", &buffer);
  printf("stat(\"file1\") result: %d\n", res);
  if (res == -1)
  {
    printf("stat error: %s\n", strerror(errno));
  }
  else {
    print_stat(buffer);
  }
  CHECKSERT(res == 0, "stat() of file that exists is ok");

  res = stat("folder666", &buffer);
  printf("stat(\"folder1\") result: %d\n", res);
  if (res == -1)
  {
    printf("stat error: %s\n", strerror(errno));
  }
  else {
    print_stat(buffer);
  }
  CHECKSERT(res == -1, "stat() of folder that does not exist fails");

  res = stat("file666", &buffer);
  printf("stat(\"file666\") result: %d\n", res);
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

  res = chdir("file2");
  printf("chdir result (not a folder): %d\n", res);
  if (res == -1)
  {
    printf("chdir error: %s\n", strerror(errno));
  }
  CHECKSERT(res == -1, "chdir() to a file should fail");

  res = chdir("/mnt/disk/folder1");
  printf("chdir result (existing folder): %d\n", res);
  if (res == -1)
  {
    printf("chdir error: %s\n", strerror(errno));
  }
  CHECKSERT(res == 0, "chdir (absolute) to folder that exists is ok");

  printf("changing dir\n");
  res = chdir("/mnt/disk/folder1");
  printf("chdir res: %d\n", res);
  res = fstatat(AT_FDCWD, "file1", &buffer, 0);
  printf("fstatat(\"file1\") result: %d\n", res);
  if (res == -1)
  {
    printf("fstatat error: %s\n", strerror(errno));
  }
  else {
    print_stat(buffer);
  }
  CHECKSERT(res == 0, "fstatat() of file that exists is ok");

  res = chdir("/folder1");
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

  res = chdir("foldera");
  printf("chdir result (to subfolder of cwd): %d\n", res);
  if (res == -1)
  {
    printf("chdir error: %s\n", strerror(errno));
  }
  CHECKSERT(res == 0, "chdir to subfolder of cwd is ok");

  /**
     If buf is a null pointer, the behavior of getcwd() is unspecified.
     http://pubs.opengroup.org/onlinepubs/9699919799/functions/getcwd.html

     Changed behavior of getcwd to Expect buf isn't nullptr.

     TODO: It's nice to have these test cases in there, but it will require
     the test to throw on contract violation
  **/

  /**
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
  **/

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
}

void print_stat(struct stat buffer)
{
  printf("st_dev: %d\n", buffer.st_dev);
  printf("st_ino: %hu\n", buffer.st_ino);
  printf("st_mode: %d\n", buffer.st_mode);
  printf("st_nlink: %d\n", buffer.st_nlink);
  printf("st_uid: %d\n", buffer.st_uid);
  printf("st_gid: %d\n", buffer.st_gid);
  printf("st_rdev: %d\n", buffer.st_rdev);
  printf("st_size: %ld\n", buffer.st_size);
  printf("st_atime: %ld\n", buffer.st_atime);
  printf("st_ctime: %ld\n", buffer.st_ctime);
  printf("st_mtime: %ld\n", buffer.st_mtime);
  printf("st_blksize: %ld\n", buffer.st_blksize);
  printf("st_blocks: %ld\n", buffer.st_blocks);
}
