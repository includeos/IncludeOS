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

#include <kernel/syscalls.hpp>
#include <kernel/os.hpp>
#include <system_log>
#include <statman>
#include <kprint>
#include <info>
#include <smp>
#include <cstring>

#if defined (UNITTESTS) && !defined(__MACH__)
#define THROW throw()
#else
#define THROW
#endif

// We can't use the usual "info", as printf isn't available after call to exit
#define SYSINFO(TEXT, ...) kprintf("%13s ] " TEXT "\n", "[ Kernel", ##__VA_ARGS__)

// Emitted if and only if panic (unrecoverable system wide error) happens
static const char* panic_signature = "\x15\x07\t**** PANIC ****";
extern uintptr_t heap_begin;
extern uintptr_t heap_end;

extern "C" __attribute__((noreturn))
void abort_message(const char* format, ...)
{
  static char abort_buf[2048];
  va_list list;
  va_start(list, format);
  vsnprintf(abort_buf, sizeof(abort_buf), format, list);
  va_end(list);
  panic(abort_buf);
}

void _exit(int status) {
  SYSINFO("Service exiting with status %d", status);
  default_exit();
  __builtin_unreachable();
}

extern "C"
void syscall_SYS_exit_group(int status)
{
  SYSINFO("Service exiting with status %d", status);
  default_exit();
  __builtin_unreachable();
}

struct alignas(SMP_ALIGN) context_buffer
{
  std::array<char, 512> buffer;
  int panics = 0;
};
static SMP::Array<context_buffer> contexts;

size_t get_crash_context_length()
{
  return PER_CPU(contexts).buffer.size();
}
char*  get_crash_context_buffer()
{
  return PER_CPU(contexts).buffer.data();
}
bool OS::is_panicking() noexcept
{
  return PER_CPU(contexts).panics > 0;
}
extern "C"
void cpu_enable_panicking()
{
  PER_CPU(contexts).panics++;
}

static OS::on_panic_func panic_handler = nullptr;
void OS::on_panic(on_panic_func func)
{
  panic_handler = std::move(func);
}

extern "C" void double_fault(const char* why);
extern "C" __attribute__((noreturn)) void panic_epilogue(const char*);
extern "C" __attribute__ ((weak))
void panic_perform_inspection_procedure() {}

/**
 * panic:
 * Display reason for kernel panic
 * Display last crash context value, if it exists
 * Display no-heap backtrace of stack
 * Call on_panic handler function, to allow application specific panic behavior
 * Print EOT character to stderr, to signal outside that PANIC output completed
 * If the handler returns, go to (permanent) sleep
**/
void panic(const char* why)
{
  cpu_enable_panicking();
  if (PER_CPU(contexts).panics > 4)
    double_fault(why);

  const int current_cpu = SMP::cpu_id();

  SMP::global_lock();

  // Tell the System log that we have paniced
  SystemLog::set_flags(SystemLog::PANIC);

  /// display informacion ...
  fprintf(stderr, "\n%s\nCPU: %d, Reason: %s\n",
          panic_signature, current_cpu, why);

  // crash context (can help determine source of crash)
  const int len = strnlen(get_crash_context_buffer(), get_crash_context_length());
  if (len > 0) {
    printf("\n\t**** CPU %d CONTEXT: ****\n %*s\n\n",
        current_cpu, len, get_crash_context_buffer());
  }

  // heap info
  typedef unsigned long ulong;
  uintptr_t heap_total = OS::heap_max() - OS::heap_begin();
  fprintf(stderr, "Heap is at: %p / %p  (diff=%lu)\n",
         (void*) OS::heap_end(), (void*) OS::heap_max(), (ulong) (OS::heap_max() - OS::heap_end()));
  fprintf(stderr, "Heap usage: %lu / %lu Kb\n", // (%.2f%%)\n",
         (ulong) (OS::heap_end() - OS::heap_begin()) / 1024,
         (ulong) heap_total / 1024); //, total * 100.0);

  print_backtrace();
  fflush(stderr);
  SMP::global_unlock();

  // action that restores some system functionality intended for inspection
  // NB: Don't call this from double faults
  panic_perform_inspection_procedure();

  panic_epilogue(why);
}

extern "C"
void double_fault(const char* why)
{
  SMP::global_lock();
  fprintf(stderr, "\n\n%s\nDouble fault! \nCPU: %d, Reason: %s\n",
          panic_signature, SMP::cpu_id(), why);
  SMP::global_unlock();

  panic_epilogue(why);
}

void panic_epilogue(const char* why)
{
  // Call custom on panic handler (if present).
  if (panic_handler != nullptr) {
    // Avoid recursion if the panic handler results in panic
    auto final_action = panic_handler;
    panic_handler = nullptr;
    final_action(why);
  }

#if defined(ARCH_x86)
  SMP::global_lock();
  // Signal End-Of-Transmission
  kprint("\x04");
  SMP::global_unlock();

#else
#warning "panic() handler not implemented for selected arch"
#endif

  switch (OS::panic_action())
  {
  case OS::Panic_action::halt:
    while (1) OS::halt();
  case OS::Panic_action::shutdown:
    extern __attribute__((noreturn)) void __arch_poweroff();
    __arch_poweroff();
    [[fallthrough]]; // needed for g++ bug
  case OS::Panic_action::reboot:
    OS::reboot();
  }

  __builtin_unreachable();
}

// Shutdown the machine when one of the exit functions are called
void default_exit() {
  __arch_poweroff();
  __builtin_unreachable();
}

extern "C"
void _init_syscalls()
{
  // make sure each buffer is zero length so it won't always show up in crashes
  for (auto& ctx : contexts)
      ctx.buffer[0] = 0;
}
