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
  if (!start) {
    start = (uintptr_t) malloc(65536);
  }
  static Statman statman_{start, 8192};
  return statman_;
}

#include <rtc>
RTC::timestamp_t RTC::booted_at = 0;

RTC::timestamp_t RTC::now() {
  return 0;
}

void RTC::init() {
  return;
}

#include <kernel/timers.hpp>
void Timers::timers_handler() {
  return;
}

void Timers::ready() {
  return;
}

void Timers::stop(int) {
  return;
}

void Timers::init(const start_func_t&, const stop_func_t&) {
  return;
}

Timers::id_t Timers::periodic(duration_t, duration_t, handler_t) {
  return 0;
}

#include <os>
void OS::resume_softreset(intptr_t) {
  return;
}

bool OS::is_softreset_magic(uint32_t) {
  return true;
}

extern "C" {

  char _binary_apic_boot_bin_end;
  char _binary_apic_boot_bin_start;
  char _ELF_START_;
  char _ELF_END_;
  uintptr_t _MULTIBOOT_START_;
  uintptr_t _LOAD_START_;
  uintptr_t _LOAD_END_;
  uintptr_t _BSS_END_;
  uintptr_t _MAX_MEM_MIB_;
#ifdef __MACH__
  uintptr_t _start;
#endif
  uintptr_t _end;

  uintptr_t get_cpu_esp() {
    return 0xdeadbeef;
  }

  void _init_c_runtime() {
    return;
  }

#ifdef __MACH__
  void _init() {
    return;
  }
#endif

  void modern_interrupt_handler() {
    return;
  }

  void unused_interrupt_handler() {
    return;
  }

  void spurious_intr() {
    return;
  }

  void lapic_send_eoi() {
    return;
  }

  void lapic_irq_entry() {
    return;
  }

  void get_cpu_id() {
    return;
  }

  void cpu_sampling_irq_entry() {
    return;
  }

  void __init_sanity_checks() noexcept {}
  void kernel_sanity_checks() {}

  void reboot_os() {
    return;
  }

  struct mallinfo { int x; };
  struct mallinfo mallinfo() {
    return {0};
  }

  void malloc_trim() {
    return;
  }
}

/// arch ///
void __arch_init() {}
void __arch_poweroff() {}
void __arch_reboot() {}
void __arch_enable_legacy_irq(uint8_t) {}
void __arch_disable_legacy_irq(uint8_t) {}

#include <smp>
int SMP::cpu_id() noexcept {
  return 0;
}
void SMP::global_lock() noexcept {}
void SMP::global_unlock() noexcept {}

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
  return true;
}
#include <kernel/cpuid.hpp>
bool CPUID::has_feature(Feature f) {
  return true;
}
#include <kernel/irq_manager.hpp>
IRQ_manager& IRQ_manager::get() {
  static IRQ_manager m;
  return m;
}
void IRQ_manager::process_interrupts() {
  return;
}
#endif

