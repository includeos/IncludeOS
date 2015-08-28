
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/times.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <stdio.h>

#include <sys/unistd.h>

extern "C"
{
  int  kill(pid_t pid, int sig);
  void panic(const char* why);
}

//Compiler says this is already declared in <sys/time.h>
//int gettimeofday(struct timeval *p, struct timezone *z);
