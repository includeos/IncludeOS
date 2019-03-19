// -*-C++-*-
// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017 Oslo and Akershus University College of Applied Sciences
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

#pragma once
#ifndef INCLUDEOS_ARCH_HEADER
#define INCLUDEOS_ARCH_HEADER

#include <cstddef>
#include <cstdint>
#include <cassert>
#include <ctime>
#include <string>


extern void __arch_poweroff();
extern void __arch_reboot();
extern void __arch_enable_legacy_irq(uint8_t);
extern void __arch_disable_legacy_irq(uint8_t);
extern void __arch_system_deactivate();
extern void __arch_install_irq(uint8_t, void(*)());
extern void __arch_subscribe_irq(uint8_t);
extern void __arch_unsubscribe_irq(uint8_t);
extern void __arch_preempt_forever(void(*)());
inline void __arch_hw_barrier() noexcept;
inline void __sw_barrier() noexcept;
extern uint64_t __arch_system_time() noexcept;
extern timespec __arch_wall_clock() noexcept;
extern uint32_t __arch_rand32();

inline void __arch_hw_barrier() noexcept {
  __sync_synchronize();
}

inline void __sw_barrier() noexcept
{
  asm volatile("" ::: "memory");
}

// Include arch specific inline implementations
#if defined(ARCH_x86_64)
#include "arch/x86_64.hpp"
#elif defined(ARCH_i686)
#include "arch/i686.hpp"
#elif defined(ARCH_aarch64)
#include "arch/aarch64.hpp"
#else
#error "Unsupported arch specified"
#endif

// retrieve system information
struct arch_system_info_t
{
  std::string uuid;
  uint64_t    physical_memory;
};
const arch_system_info_t& __arch_system_info() noexcept;

#endif
