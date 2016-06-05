// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
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

#include <fcntl.h> // open()
#include <string.h>
#include <signal.h>

#include <sys/errno.h>
#include <sys/stat.h>

#include <os>
#include <kernel/syscalls.hpp>

char*   __env[1] {nullptr};
char**  environ {__env};
caddr_t heap_end;

static const int syscall_fd {999};
static bool debug_syscalls  {true};


void _exit(int status) {
  (void) status;
  panic("Exit called");
}

int close(int) {
  panic("SYSCALL CLOSE NOT SUPPORTED");
  return -1;
};

int execve(const char* UNUSED(name),
           char* const* UNUSED(argv),
           char* const* UNUSED(env))
{
  panic("SYSCALL EXECVE NOT SUPPORTED");
  return -1;
};

int fork() {
  panic("SYSCALL FORK NOT SUPPORTED");
  return -1;
};

int fstat(int UNUSED(file), struct stat *st) {
  debug("SYSCALL FSTAT Dummy, returning OK 0");
  st->st_mode = S_IFCHR;
  return 0;
};

int getpid() {
  debug("SYSCALL GETPID Dummy, returning 1");
  return 1;
};

int isatty(int file) {
  if (file == 1 || file == 2 || file == 3) {
    debug("SYSCALL ISATTY Dummy returning 1");
    return 1;
  }

  // Not stdxxx, error out
  panic("SYSCALL ISATTY Unknown descriptor ");
  errno = EBADF;
  return 0;
}

int link(const char* UNUSED(old), const char* UNUSED(_new)) {
  panic("SYSCALL LINK unsupported");
  return -1;
}

int unlink(const char* UNUSED(name)) {
  panic("SYSCALL UNLINK unsupported");
  return -1;
}

off_t lseek(int UNUSED(file), off_t UNUSED(ptr), int UNUSED(dir)) {
  panic("SYSCALL LSEEK returning 0");
  return 0;
}

int open(const char* UNUSED(name), int UNUSED(flags), ...) {
  panic("SYSCALL OPEN unsupported");
  return -1;
};

int read(int UNUSED(file), void* UNUSED(ptr), size_t UNUSED(len)) {
  panic("SYSCALL READ unsupported");
  return 0;
}

int write(int file, const void* ptr, size_t len) {
  if (file == syscall_fd and not debug_syscalls) {
    return len;
  }
  return OS::rsprint((const char*) ptr, len);
}

void* sbrk(ptrdiff_t incr) {
  void* prev_heap_end = heap_end;
  heap_end += incr;
  return (caddr_t) prev_heap_end;
}


int stat(const char* UNUSED(file), struct stat *st) {
  debug("SYSCALL STAT Dummy");
  st->st_mode = S_IFCHR;
  return 0;
};

clock_t times(struct tms* UNUSED(buf)) {
  panic("SYSCALL TIMES Dummy, returning -1");
  return -1;
};

int wait(int* UNUSED(status)) {
  debug((char*)"SYSCALL WAIT Dummy, returning -1");
  return -1;
};

int gettimeofday(struct timeval* p, void* UNUSED(z)) {
  // Currently every reboot takes us back to 1970 :-)
  float seconds = OS::uptime();
  p->tv_sec = int(seconds);
  p->tv_usec = (seconds - p->tv_sec) * 1000000;
  return 5;
}

int kill(pid_t pid, int sig) {
  printf("!!! Kill PID: %i, SIG: %i - %s ", pid, sig, strsignal(sig));

  if (sig == 6ul) {
    printf("/ ABORT\n");
  }

  panic("\tKilling a process doesn't make sense in IncludeOS. Panic.");
  errno = ESRCH;
  return -1;
}

// No continuation from here
void panic(const char* why) {
  printf("\n\t **** PANIC: ****\n %s\n", why);
  printf("\tHeap end: %p\n", heap_end);
  print_backtrace();
  while(1) __asm__("cli; hlt;");
}

// No continuation from here
void default_exit() {
  panic("Exit was called");
}

// To keep our sanity, we need a reason for the abort
void abort_ex(const char* why) {
  printf("\n\t !!! abort_ex. Why: %s", why);
  panic(why);
}
