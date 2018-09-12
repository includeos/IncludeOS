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

#include "apic_timer.hpp"
#include "apic.hpp"
#include "pit.hpp"
#include <kernel/events.hpp>
#include <kernel/timers.hpp>
#include <smp>
#include <cstdio>
#include <info>

#define TIMER_ONESHOT     0x0
#define TIMER_PERIODIC    0x20000
#define TIMER_DEADLINE    0x40000

// vol 3a  10-10
#define DIVIDE_BY_16     0x3

#define CALIBRATION_MS   125

using namespace std::chrono;

namespace x86
{
  // calculated once on BSP
  static uint32_t ticks_per_micro = 0;

  struct alignas(SMP_ALIGN) timer_data
  {
    int  intr;
    bool intr_enabled = false;
  };
  static SMP::Array<timer_data> timerdata;

  #define GET_TIMER() PER_CPU(timerdata)

  void APIC_Timer::init()
  {
    // initialize timer system
    Timers::init(
        oneshot, // timer start function
        stop);   // timer stop function

    // set interrupt handler
    GET_TIMER().intr =
        Events::get().subscribe(Timers::timers_handler);
    // initialize local APIC timer
    APIC::get().timer_init(GET_TIMER().intr);
  }
  void APIC_Timer::calibrate()
  {
    init();

    if (ticks_per_micro != 0) {
      start_timers();
      // with SMP, signal everyone else too (IRQ 1)
      if (SMP::cpu_count() > 1) {
        APIC::get().bcast_ipi(0x21);
      }
      return;
    }

    // start timer (unmask)
    INFO("APIC", "Measuring APIC timer...");

    auto& lapic = APIC::get();
    // See: Vol3a 10.5.4.1 TSC-Deadline Mode
    // 0xFFFFFFFF --> ~68 seconds
    // 0xFFFFFF   --> ~46 milliseconds
    lapic.timer_begin(0xFFFFFFFF);
    // measure function call and tick read overhead
    uint32_t overhead;
    [&overhead] {
        overhead = APIC::get().timer_diff();
    }();
    // restart counter
    lapic.timer_begin(0xFFFFFFFF);

    /// use PIT to measure <time> in one-shot ///
    PIT::oneshot(milliseconds(CALIBRATION_MS),
    [overhead] {
      uint32_t diff = APIC::get().timer_diff() - overhead;
      assert(ticks_per_micro == 0);
      // measure difference
      ticks_per_micro = diff / CALIBRATION_MS / 1000;
      // stop APIC timer
      APIC::get().timer_interrupt(false);

      //printf("* APIC timer: ticks %ums: %u\t 1mi: %u\n",
      //       CALIBRATION_MS, diff, ticks_per_micro);
      start_timers();

      // with SMP, signal everyone else too (IRQ 1)
      if (SMP::cpu_count() > 1) {
        APIC::get().bcast_ipi(0x21);
      }
    });
  }

  void APIC_Timer::start_timers() noexcept
  {
    assert(ready());
    // delay-start all timers
    Events::get().defer(Timers::ready);
  }

  bool APIC_Timer::ready() noexcept
  {
    return ticks_per_micro != 0;
  }

  void APIC_Timer::oneshot(std::chrono::nanoseconds nanos) noexcept
  {
    // prevent overflow
    uint64_t ticks = nanos.count() / 1000 * ticks_per_micro;
    if (ticks > 0xFFFFFFFF)
        ticks = 0xFFFFFFFF;
    // prevent oneshots less than a microsecond
    // NOTE: when ticks == 0, the entire timer system stops
    else if (UNLIKELY(ticks < ticks_per_micro))
        ticks = ticks_per_micro;

    // set initial counter
    auto& lapic = APIC::get();
    lapic.timer_begin(ticks);
    // re-enable interrupts if disabled
    if (GET_TIMER().intr_enabled == false) {
      GET_TIMER().intr_enabled = true;
      lapic.timer_interrupt(true);
    }
  }
  void APIC_Timer::stop() noexcept
  {
    GET_TIMER().intr_enabled = false;
    APIC::get().timer_interrupt(false);
  }

  // used by soft-reset
  uint32_t apic_timer_get_ticks() noexcept
  {
    return ticks_per_micro;
  }
  void     apic_timer_set_ticks(uint32_t tpm) noexcept
  {
    ticks_per_micro = tpm;
  }
}
