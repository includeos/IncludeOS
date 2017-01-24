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
#include <hw/serial.hpp>

#include <statman>
#include <kprint>
#include <info>


#if defined (UNITTESTS) && !defined(__MACH__)
#define THROW throw()
#else
#define THROW
#endif

// We can't use the usual "info", as printf isn't available after call to exit
#define SYSINFO(TEXT, ...) kprintf("%13s ] " TEXT "\n", "[ Kernel", ##__VA_ARGS__)

#define SHUTDOWN_ON_PANIC 1

char*   __env[1] {nullptr};
char**  environ {__env};
extern "C" {
  uintptr_t heap_begin;
  uintptr_t heap_end;
}

void _exit(int status) {
  kprintf("%s",std::string(LINEWIDTH, '=').c_str());
  kprint("\n");
  SYSINFO("service exited with status %i", status);
  default_exit();
}

int execve(const char*,
           char* const*,
           char* const*)
{
  panic("SYSCALL EXECVE NOT SUPPORTED");
  return -1;
}

int fork() {
  panic("SYSCALL FORK NOT SUPPORTED");
  return -1;
}

int fstat(int, struct stat* st) {
  debug("SYSCALL FSTAT Dummy, returning OK 0");
  st->st_mode = S_IFCHR;
  return 0;
}

int getpid() {
  debug("SYSCALL GETPID Dummy, returning 1");
  return 1;
}

int link(const char*, const char*) {
  panic("SYSCALL LINK unsupported");
  return -1;
}

int unlink(const char*) {
  panic("SYSCALL UNLINK unsupported");
  return -1;
}

void* sbrk(ptrdiff_t incr) {
  /// NOTE:
  /// sbrk gets called really early on, before everything else
  if (UNLIKELY(heap_end + incr > OS::heap_max())) {
    errno = ENOMEM;
    return (void*)-1;
  }
  auto prev_heap_end = heap_end;
  heap_end += incr;
  return (void*) prev_heap_end;
}

/*
int stat(const char*, struct stat *st) {
  debug("SYSCALL STAT Dummy");
  st->st_mode = S_IFCHR;
  return 0;
}
*/

clock_t times(struct tms*) {
  panic("SYSCALL TIMES Dummy, returning -1");
  return -1;
}

int wait(int*) {
  debug((char*)"SYSCALL WAIT Dummy, returning -1");
  return -1;
}

int gettimeofday(struct timeval* p, void*) {
  p->tv_sec  = RTC::now();
  p->tv_usec = 0;
  return 0;
}

int kill(pid_t pid, int sig) THROW {
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

static void default_panic_handler()
{
  // shutdown the machine
  if (SHUTDOWN_ON_PANIC)
      hw::ACPI::shutdown();
}
OS::on_panic_func panic_handler = default_panic_handler;

/**
 * panic:
 * Display reason for kernel panic
 * Display last crash context value, if it exists
 * Display no-heap backtrace of stack
 * Print EOT character to stderr, to signal outside that PANIC occured
 * Call on_panic handler function, which determines what to do when
 *    the kernel panics
 * If the handler returns, go to (permanent) sleep
**/
void panic(const char* why) {
  fprintf(stderr, "\n\t**** PANIC: ****\n %s\n", why);
  // the crash context buffer can help determine cause of crash
  int len = strnlen(get_crash_context_buffer(), CONTEXT_BUFFER_LENGTH);
  if (len > 0) {
    printf("\n\t**** CONTEXT: ****\n %*s\n\n",
        len, get_crash_context_buffer());
  }
  // heap and backtrace info
  uintptr_t heap_total = OS::heap_max() - heap_begin;
  double total = (heap_end - heap_begin) / (double) heap_total;
  
  fprintf(stderr, "\tHeap is at: %#x / %#x  (diff=%#x)\n",
         heap_end, OS::heap_max(), OS::heap_max() - heap_end);
  fprintf(stderr, "\tHeap usage: %u / %u Kb (%.2f%%)\n",
         (uintptr_t) (heap_end - heap_begin) / 1024, 
         heap_total / 1024,
         total * 100.0);
  print_backtrace();

  // Signal End-Of-Transmission
  fprintf(stderr, "\x04"); fflush(stderr);
  
  // call on_panic handler
  panic_handler();
  
  // .. if we return from the panic handler, go to permanent sleep
  while (1) asm("cli; hlt");
  __builtin_unreachable();
}

// Shutdown the machine when one of the exit functions are called
void default_exit() {
  hw::ACPI::shutdown();
  __builtin_unreachable();
}

// To keep our sanity, we need a reason for the abort
void abort_ex(const char* why) {
  panic(why);
  __builtin_unreachable();
}

// Basic second-resolution implementation - using CMOS directly for now.
int clock_gettime(clockid_t clk_id, struct timespec* tp) {
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
