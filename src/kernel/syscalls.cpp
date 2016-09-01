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

#include <kernel/os.hpp>
#include <kernel/syscalls.hpp>
#include <kernel/rtc.hpp>

#include <hw/acpi.hpp>

#include <statman>

char*   __env[1] {nullptr};
char**  environ {__env};
extern "C" {
  uintptr_t heap_begin;
  uintptr_t heap_end;
}

static const int syscall_fd   {999};
static bool debug_syscalls    {true};
static uint32_t& sbrk_called  {Statman::get().create(Stat::UINT32, "syscalls.sbrk").get_uint32()};

void _exit(int) {
  default_exit();
}

int close(int) {
  panic("SYSCALL CLOSE NOT SUPPORTED");
  return -1;
}

int execve(const char* UNUSED(name),
           char* const* UNUSED(argv),
           char* const* UNUSED(env))
{
  panic("SYSCALL EXECVE NOT SUPPORTED");
  return -1;
}

int fork() {
  panic("SYSCALL FORK NOT SUPPORTED");
  return -1;
}

int fstat(int UNUSED(file), struct stat *st) {
  debug("SYSCALL FSTAT Dummy, returning OK 0");
  st->st_mode = S_IFCHR;
  return 0;
}

int getpid() {
  debug("SYSCALL GETPID Dummy, returning 1");
  return 1;
}

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
  // Stat increment syscall sbrk called
  sbrk_called++;

  if (UNLIKELY(heap_end + incr > OS::heap_max())) {
    errno = ENOMEM;
    return (void*)-1;
  }
  auto prev_heap_end = heap_end;
  heap_end += incr;
  return (void*) prev_heap_end;
}


int stat(const char* UNUSED(file), struct stat *st) {
  debug("SYSCALL STAT Dummy");
  st->st_mode = S_IFCHR;
  return 0;
}

clock_t times(struct tms* UNUSED(buf)) {
  panic("SYSCALL TIMES Dummy, returning -1");
  return -1;
}

int wait(int* UNUSED(status)) {
  debug((char*)"SYSCALL WAIT Dummy, returning -1");
  return -1;
}

int gettimeofday(struct timeval* p, void*) {
  p->tv_sec  = RTC::now();
  p->tv_usec = 0;
  return 0;
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

static const size_t CONTEXT_BUFFER_LENGTH = 0x1000;
static char _crash_context_buffer[CONTEXT_BUFFER_LENGTH] __attribute__((aligned(0x1000)));

size_t get_crash_context_length()
{
  return CONTEXT_BUFFER_LENGTH;
}
char*  get_crash_context_buffer()
{
  return _crash_context_buffer;
}


// No continuation from here
void panic(const char* why) {
  printf("\n\t**** PANIC: ****\n %s\n", why);
  // the crash context buffer can help determine cause of crash
  int len = strnlen(get_crash_context_buffer(), CONTEXT_BUFFER_LENGTH);
  if (len > 0) {
    printf("\n\t**** CONTEXT: ****\n %*s\n\n",
        len, get_crash_context_buffer());
  }
  // heap and backtrace info
  extern char _end;
  printf("\tHeap end: %#x (heap %u Kb, max %u Kb)\n",
         heap_end, (uintptr_t) (heap_end - heap_begin) / 1024, (uintptr_t) heap_end / 1024);
  print_backtrace();
  // shutdown the machine
  hw::ACPI::shutdown();
  while (1) asm("cli; hlt");
}

// Shutdown the machine when one of the exit functions are called
void default_exit() {
  hw::ACPI::shutdown();
  while (1) asm("cli; hlt");
}

// To keep our sanity, we need a reason for the abort
void abort_ex(const char* why) {
  panic(why);
}

// Basic second-resolution implementation - using CMOS directly for now.
int clock_gettime(clockid_t clk_id, struct timespec *tp){
  if (clk_id == CLOCK_REALTIME) {
    tp->tv_sec = RTC::now();
    tp->tv_nsec = 0;
    return 0;
  }
  return -1;
}

extern "C" void _init_syscalls();
void _init_syscalls()
{
  // make sure that the buffers length is zero so it won't always show up in crashes
  _crash_context_buffer[0] = 0;
}
