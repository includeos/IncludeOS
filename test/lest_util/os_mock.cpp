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
#ifdef __MACH__
#include <stdlib.h>
#include <stddef.h>
#include <gsl/gsl_assert>
void* memalign(size_t alignment, size_t size) {
  void* ptr {nullptr};
  int res = posix_memalign(&ptr, alignment, size);
  Ensures(res == 0);
  return ptr;
}
void* aligned_alloc(size_t alignment, size_t size) {
  return memalign(alignment, size);
}
#endif

#include <util/statman.hpp>
Statman& Statman::get() {
  static uintptr_t start {0};
  static const size_t memsize = 0x1000000;
  if (!start) {
    start = (uintptr_t) malloc(memsize);
  }
  static Statman statman_{start, memsize / sizeof(Stat)};
  return statman_;
}

/// RTC ///
#include <rtc>
RTC::timestamp_t RTC::booted_at = 0;
void RTC::init() {}

/// TIMERS ///
#include <kernel/timers.hpp>
void Timers::timers_handler() {}
void Timers::ready() {}
void Timers::stop(int) {}
void Timers::init(const start_func_t&, const stop_func_t&) {}
Timers::id_t Timers::periodic(duration_t, duration_t, handler_t) {
  return 0;
}

#include <service>
const char* service_binary_name__ = "Service binary name";
const char* service_name__        = "Service name";

#include <kernel/os.hpp>
void OS::start(unsigned, unsigned) {}
void OS::default_stdout(const char*, size_t) {}
void OS::event_loop() {}
void OS::block() {}
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
extern "C" {

/// Kernel ///

  char _binary_apic_boot_bin_end;
  char _binary_apic_boot_bin_start;
  char _ELF_START_;
  char _ELF_END_;
  uintptr_t _MULTIBOOT_START_;
  uintptr_t _LOAD_START_;
  uintptr_t _LOAD_END_;
  uintptr_t _BSS_END_;
  uintptr_t _TEXT_START_;
  uintptr_t _TEXT_END_;
  uintptr_t _EXEC_END_;

  uintptr_t get_cpu_esp() {
    return 0xdeadbeef;
  }

/// C ABI ///
  void _init_c_runtime() {}
  void _init_bss() {}
  void _init_heap(uintptr_t) {}

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

  __attribute__((weak))
  void __init_serial1 () {}
  __attribute__((weak))
  void __serial_print1(const char* cstr) {
    static char __printbuf[4096];
    snprintf(__printbuf, sizeof(__printbuf), "%s", cstr);
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

uint64_t __arch_system_time() noexcept {
  return 0;
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

#endif
