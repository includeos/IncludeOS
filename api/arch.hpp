// -*-C++-*-

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
extern void __arch_subscribe_irq(uint8_t, int cpu = 0);
extern void __arch_unsubscribe_irq(uint8_t, int cpu = 0);
extern void __arch_preempt_forever(void(*)());
extern uint64_t __arch_system_time() noexcept;
extern timespec __arch_wall_clock() noexcept;
extern uint32_t __arch_rand32();

inline void compiler_barrier() noexcept
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
