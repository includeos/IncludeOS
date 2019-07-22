

#pragma once
#ifndef X86_APIC_TIMER_HPP
#define X86_APIC_TIMER_HPP

#include <chrono>

namespace x86 {

struct APIC_Timer
{
  static void init();
  static void calibrate();
  static void start_timers() noexcept;

  static bool ready() noexcept;

  static void oneshot(std::chrono::nanoseconds) noexcept;
  static void stop() noexcept;
};

}

#endif
