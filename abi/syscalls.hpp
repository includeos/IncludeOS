
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/times.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <stdio.h>

// _exit, sbrk, open, ...
#include <sys/unistd.h>

extern "C"
{
  void panic(const char* why);
}
