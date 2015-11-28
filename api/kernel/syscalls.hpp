#ifndef KERNEL_SYSCALLS_HPP
#define KERNEL_SYSCALLS_HPP

#include <sys/unistd.h>

extern "C"
{
  int  kill(pid_t pid, int sig);
  void panic(const char* why) __attribute__((noreturn));
}

#endif
