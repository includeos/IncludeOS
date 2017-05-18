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
#include <hw/serial.hpp>

#include <statman>
#include <kprint>
#include <info>
#include <smp>

#if defined (UNITTESTS) && !defined(__MACH__)
#define THROW throw()
#else
#define THROW
#endif

// We can't use the usual "info", as printf isn't available after call to exit
#define SYSINFO(TEXT, ...) kprintf("%13s ] " TEXT "\n", "[ Kernel", ##__VA_ARGS__)

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
static char _crash_context_buffer[CONTEXT_BUFFER_LENGTH];

size_t get_crash_context_length()
{
  return CONTEXT_BUFFER_LENGTH;
}
char*  get_crash_context_buffer()
{
  return _crash_context_buffer;
}

static bool panic_reenter = false;
static OS::on_panic_func panic_handler = nullptr;

void OS::on_panic(on_panic_func func)
{
  panic_handler = std::move(func);
}

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
void panic(const char* why)
{
  /// prevent re-entering panic() more than once per CPU
  //if (panic_reenter) OS::reboot();
  panic_reenter = true;

  /// display informacion ...
  SMP::global_lock();
  fprintf(stderr, "\n\t**** CPU %d PANIC: ****\n %s\n",
          SMP::cpu_id(), why);

  // crash context (can help determine source of crash)
  int len = strnlen(get_crash_context_buffer(), CONTEXT_BUFFER_LENGTH);
  if (len > 0) {
    printf("\n\t**** CONTEXT: ****\n %*s\n",
        len, get_crash_context_buffer());
  }
  fprintf(stderr, "\n");

#if defined(ARCH_x86_64)
  // CPU registers
  uintptr_t regs[24];
  asm ("movq %%rax, %0" : "=a" (regs[0]));
  asm ("movq %%rbx, %0" : "=b" (regs[1]));
  asm ("movq %%rcx, %0" : "=c" (regs[2]));
  asm ("movq %%rdx, %0" : "=d" (regs[3]));
  asm ("movq %%rbp, %0" : "=a" (regs[4]));

  asm ("movq %%r8, %0"  : "=a" (regs[5]));
  asm ("movq %%r9, %0"  : "=b" (regs[6]));
  asm ("movq %%r10, %0" : "=c" (regs[7]));
  asm ("movq %%r11, %0" : "=d" (regs[8]));
  asm ("movq %%r12, %0" : "=a" (regs[9]));
  asm ("movq %%r13, %0" : "=b" (regs[10]));
  asm ("movq %%r14, %0" : "=c" (regs[11]));
  asm ("movq %%r15, %0" : "=d" (regs[12]));

  asm ("movq %%rsp, %0" : "=a" (regs[13]));
  asm ("movq %%rsi, %0" : "=b" (regs[14]));
  asm ("movq %%rdi, %0" : "=c" (regs[15]));
  asm ("movq %%rip, %0" : "=d" (regs[16]));

  asm ("pushf; popq %0" : "=a" (regs[17]));
  asm ("movq %%cr0, %0" : "=b" (regs[18]));
  /*
  asm ("movq %%cr1, %0" : "=r" (regs[19]));
  */
  asm ("movq %%cr2, %0" : "=c" (regs[20]));
  asm ("movq %%cr3, %0" : "=d" (regs[21]));
  asm ("movq %%cr4, %0" : "=a" (regs[22]));
  asm ("movq %%cr8, %0" : "=b" (regs[23]));

  struct desc_table_t {
    uint16_t  limit;
    uintptr_t location;
  } __attribute__((packed))  gdt, idt;
  asm ("sgdtq %0" : : "m" (* &gdt));
  asm ("sidtq %0" : : "m" (* &idt));

  printf("  RAX:  %016lx  R 8:  %016lx\n", regs[0], regs[5]);
  printf("  RBX:  %016lx  R 9:  %016lx\n", regs[1], regs[6]);
  printf("  RCX:  %016lx  R10:  %016lx\n", regs[2], regs[7]);
  printf("  RDX:  %016lx  R11:  %016lx\n", regs[3], regs[8]);
  fprintf(stderr, "\n");

  printf("  RBP:  %016lx  R12:  %016lx\n", regs[4], regs[9]);
  printf("  RSP:  %016lx  R13:  %016lx\n", regs[13], regs[10]);
  printf("  RSI:  %016lx  R14:  %016lx\n", regs[14], regs[11]);
  printf("  RDI:  %016lx  R15:  %016lx\n", regs[15], regs[12]);
  printf("  RIP:  %016lx  FLA:  %016lx\n", regs[16], regs[17]);
  fprintf(stderr, "\n");

  printf("  CR0:  %016lx  CR4:  %016lx\n", regs[18], regs[22]);
  printf("  CR1:  %016lx  CR8:  %016lx\n", regs[19], regs[23]);
  printf("  CR2:  %016lx  GDT:  %016lx (%u)\n", regs[20], gdt.location, gdt.limit);
  printf("  CR3:  %016lx  IDT:  %016lx (%u)\n", regs[21], idt.location, idt.limit);
  fprintf(stderr, "\n");

#elif defined(ARCH_i686)
  // CPU registers
  uintptr_t regs[16];
  asm ("movl %%eax, %0" : "=r" (regs[0]));
  asm ("movl %%ebx, %0" : "=r" (regs[1]));
  asm ("movl %%ecx, %0" : "=r" (regs[2]));
  asm ("movl %%edx, %0" : "=r" (regs[3]));

  asm ("movl %%ebp, %0" : "=r" (regs[4]));
  asm ("movl %%esp, %0" : "=r" (regs[5]));
  asm ("movl %%esi, %0" : "=r" (regs[6]));
  asm ("movl %%edi, %0" : "=r" (regs[7]));

  asm ("movl (%%esp), %0" : "=r" (regs[8]));
  asm ("pushf; popl %0" : "=r" (regs[9]));

  printf("  EAX:  %08x  EBP:  %08x\n", regs[0], regs[4]);
  printf("  EBX:  %08x  ESP:  %08x\n", regs[1], regs[5]);
  printf("  ECX:  %08x  ESI:  %08x\n", regs[2], regs[6]);
  printf("  EDX:  %08x  EDI:  %08x\n", regs[3], regs[7]);
  printf("  EIP:  %08x  EFL:  %08x\n", regs[8], regs[9]);
  fprintf(stderr, "\n");

#else
  #error "Implement me"
#endif

  // heap info
  typedef unsigned long ulong;
  uintptr_t heap_total = OS::heap_max() - heap_begin;
  double total = (heap_end - heap_begin) / (double) heap_total;
  fprintf(stderr, "\tHeap is at: %p / %p  (diff=%lu)\n",
         (void*) heap_end, (void*) OS::heap_max(), (ulong) (OS::heap_max() - heap_end));
  fprintf(stderr, "\tHeap usage: %lu / %lu Kb (%.2f%%)\n",
         (ulong) (heap_end - heap_begin) / 1024,
         (ulong) heap_total / 1024, total * 100.0);

  // call stack
  print_backtrace();

  fflush(stderr);
  SMP::global_unlock();

  // call custom on panic handler (if present)
  if (panic_handler) panic_handler();

#if defined(ARCH_x86)
  if (SMP::cpu_id() == 0) {
    SMP::global_lock();
    // Signal End-Of-Transmission
    kprint("\x04");
    SMP::global_unlock();
  }

  // .. if we return from the panic handler, go to permanent sleep
  while (1) asm("cli; hlt");
  __builtin_unreachable();
#else
  #warning "panic() handler not implemented for selected arch"
#endif
}

// Shutdown the machine when one of the exit functions are called
void default_exit() {
  __arch_poweroff();
  __builtin_unreachable();
}

// To keep our sanity, we need a reason for the abort
void abort_ex(const char* why) {
  panic(why);
  __builtin_unreachable();
}

#if defined(__MACH__)
#if !defined(__MAC_10_12)
typedef int clockid_t;
#endif
#if !defined(CLOCK_REALTIME)
#define CLOCK_REALTIME 0
#endif
#endif
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
