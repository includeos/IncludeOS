// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2017 Oslo and Akershus University College of Applied Sciences
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

#include <unistd.h>
#include <stdarg.h>
#ifdef __MACH__
#include <stdlib.h>
#include <stddef.h>
#include <gsl/gsl_assert>
#include <errno.h>

void* memalign(size_t align, size_t size) {
  void* ptr {nullptr};

  if (align < sizeof(void*))
    align = sizeof(void*);
  if (size < sizeof(void*))
    size = sizeof(void*);

  int res = posix_memalign(&ptr, align, size);
  if (res == EINVAL)
    printf("Error %i: posix_memalign got invalid alignment param %zu \n", res, align);
  if (res == ENOMEM)
    printf("Error %i: posix_memalign failed, not enough memory  %zu \n", res);
  return ptr;
}
void* aligned_alloc(size_t align, size_t size) {
  return memalign(align, size);
}
#endif


char _DISK_START_;
char _DISK_END_;
char _ELF_SYM_START_;

/// RTC ///
#include <rtc>
RTC::timestamp_t RTC::booted_at = 0;
void RTC::init() {}

#include <service>
const char* service_binary_name__ = "Service binary name";
const char* service_name__        = "Service name";

void Service::ready()
{
  printf("Service::ready() called\n");
}

extern "C"
void kprintf(char* format, ...)
{
  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
}

extern "C"
void kprint(char* str)
{
printf("%s", str);
}

#include <kernel/os.hpp>
void OS::start(unsigned, unsigned) {}
void OS::default_stdout(const char*, size_t) {}
void OS::event_loop() {}
void OS::halt() {}
void OS::resume_softreset(intptr_t) {}
bool OS::is_softreset_magic(uint32_t) {
  return true;
}

void __x86_init_paging(void*){};
namespace x86 {
namespace paging {
  void invalidate(void* pageaddr){};
}}

__attribute__((constructor))
void paging_test_init(){
  extern uintptr_t __exec_begin;
  extern uintptr_t __exec_end;
  __exec_begin = 0xa00000;
  __exec_end = 0xb0000b;
}

void OS::multiboot(unsigned) {}

#include <system_log>
void SystemLog::initialize() {}
void SystemLog::set_flags(uint32_t) {}

/// Kernel ///
char _binary_apic_boot_bin_end;
char _binary_apic_boot_bin_start;
char __plugin_ctors_start;
char __plugin_ctors_end;
char __service_ctors_start;
char __service_ctors_end;

char _ELF_START_;
char _ELF_END_;
uintptr_t _MULTIBOOT_START_;
uintptr_t _LOAD_START_;
uintptr_t _LOAD_END_;
uintptr_t _BSS_END_;
uintptr_t _TEXT_START_;
uintptr_t _TEXT_END_;
uintptr_t _EXEC_END_;

extern "C" {
  uintptr_t get_cpu_esp() {
    return 0xdeadbeef;
  }

/// C ABI ///
  void _init_c_runtime() {}
  void _init_bss() {}

#ifdef __MACH__
  void _init() {}
#endif

  void __libc_init_array () {}

  uintptr_t _multiboot_free_begin(uintptr_t) {
    return 0;
  }
  uintptr_t _move_symbols(uintptr_t) {
    return 0;
  }

  /// malloc ///
  struct mallinfo { int x; };
  struct mallinfo mallinfo() {
    return {0};
  }
  void malloc_trim() {}
  uintptr_t heap_end = std::numeric_limits<uintptr_t>::max();

  void __init_serial1 () {}

  void __serial_print1(const char* cstr) {
    printf("<serial print1> %s\n", cstr);
  }

  void __serial_print(const char* cstr, int len) {
    printf("<serial print> %.*s", len, cstr);
  }
} // ~ extern "C"

/// platform ///
void* __multiboot_addr;

void __platform_init() {}
extern "C" void __init_sanity_checks() {}
extern "C" void kernel_sanity_checks() {}

/// arch ///
void __arch_poweroff() {}
void __arch_reboot() {}
void __arch_subscribe_irq(uint8_t) {}
void __arch_enable_legacy_irq(uint8_t) {}
void __arch_disable_legacy_irq(uint8_t) {}
void __arch_system_deactivate() {}

delegate<uint64_t()> systime_override = [] () -> uint64_t { return 0; };
uint64_t __arch_system_time() noexcept {
  return systime_override();
}
#include <sys/time.h>
timespec __arch_wall_clock() noexcept {
  return timespec{0, 0};
}

/// smp ///
#include <smp>
int SMP::cpu_id() noexcept {
  return 0;
}
void SMP::global_lock() noexcept {}
void SMP::global_unlock() noexcept {}
void SMP::add_task(SMP::task_func func, int) { func(); }
void SMP::signal(int) {}

extern "C"
void (*current_eoi_mechanism) () = nullptr;

#ifdef ARCH_X86
#include "../../src/arch/x86/apic.hpp"
namespace x86 {
  IApic& APIC::get() noexcept { return *(IApic*) 0; }
}
#endif

#ifndef ARCH_X86
bool rdrand32(uint32_t* result) {
  *result = rand();
  return true;
}

/// heap ///
uintptr_t __brk_max = 0;
uintptr_t OS::heap_begin() noexcept {
  return 0;
}

uintptr_t OS::memory_end_ = 1 << 30;

uintptr_t OS::heap_end() noexcept {
  return memory_end_;
}

size_t OS::heap_usage() noexcept {
  return OS::heap_end();
}

uintptr_t OS::heap_max() noexcept {
  return -1;
}

size_t OS::total_memuse() noexcept {
  return heap_end();
}

#endif
