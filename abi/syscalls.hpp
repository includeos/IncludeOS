
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/times.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <stdio.h>

#include <sys/unistd.h>
/*
extern "C"
{
  void _exit(int status) __attribute__ ((__noreturn__));
  int close(int file);
  int execve(const char* name, char* const* argv, char* const* env);
  int fork();
  int fstat(int file, struct stat *st);
  int getpid();
  int isatty(int file);
  int kill(int pid, int sig);
  int link(const char *old, const char *_new);
  int unlink(const char *name);
  off_t lseek(int file, off_t ptr, int dir);
  int open(const char *name, int flags, ...);
  int read(int file, void *ptr, size_t len);
  int write(int file, const void *ptr, size_t len);
  void* sbrk(ptrdiff_t incr);
  int stat(const char *file, struct stat *st);
  clock_t times(struct tms *buf);
  int wait(int *status);
}*/

extern "C"
{
  void panic(const char* why);
}

//Compiler says this is allready declared in <sys/time.h>
//int gettimeofday(struct timeval *p, struct timezone *z);
