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

#pragma once
#ifndef X86_PIT_HPP
#define X86_PIT_HPP
#include <delegate>
#include <chrono>
#include <util/units.hpp>

namespace x86
{
  /**
   * Programmable Interval Timer
   *
  **/

  using namespace util::literals;

  class PIT {
  public:
    using timeout_handler = delegate<void()>;

    // The PIT-chip runs at this fixed frequency (in MHz) */
    static constexpr MHz FREQUENCY = MHz(14.31818 / 12);

    // The closest we can get to a millisecond interval, with the PIT-frequency
    static constexpr uint16_t MILLISEC_INTERVAL = KHz(FREQUENCY).count();

    /** Create a one-shot timer.
        @param ms: Expiration time. Compatible with all std::chrono durations.
        @param handler: A delegate or function to be called on timeout.   */
    static void oneshot(std::chrono::milliseconds ms, timeout_handler handler);

    /** Run a custom handler forever */
    static void forever(timeout_handler handler);

    /** Stop PIT interrupts */
    static void stop();

    /** Return calculated frequency based on divider setting */
    static inline MHz current_frequency()
    {
      return FREQUENCY / get().current_freq_divider_;
    }

    /** Estimate cpu frequency based on the fixed PIT frequency and rdtsc.
        @Note This is an asynchronous function.  */
    static double estimate_CPU_frequency();

    /** Halt until PIT interrupt is triggered (one PIT cycle) **/
    static void blocking_cycles(int cnt);

    /** Get the (single) instance. */
    static PIT& get() {
      static PIT instance_;
      return instance_;
    }

  private:
    enum Mode { ONE_SHOT = 0,
                HW_ONESHOT = 1 << 1,
                RATE_GEN = 2 << 1,
                SQ_WAVE = 3 << 1,
                SW_STROBE = 4 << 1,
                HW_STROBE = 5 << 1,
                NONE = 256};

    /** Disable regular timer interrupts- which are ON at boot-time. */
    void disable_regular_interrupts();

    /**  The default (soft)handler for timer interrupts */
    void irq_handler();

    // State-keeping
    uint16_t current_freq_divider_ = 0;
    Mode     current_mode_ = NONE;

    // Timer handler & expiration timestamp
    timeout_handler          handler = nullptr;
    timeout_handler          forev_handler = nullptr;
    std::chrono::nanoseconds expiration;

    // Access mode bits are bits 4- and 5 in the Mode register
    enum AccessMode { LATCH_COUNT = 0x0, LO_ONLY=0x10, HI_ONLY=0x20, LO_HI=0x30 };

    /** Physically set the PIT-mode */
    void set_mode(Mode);

    /** Physiclally set the PIT frequency divider */
    void set_freq_divider(uint16_t);

    /** Set mode to one-shot, and frequency-divider to t */
    void enable_oneshot(uint16_t t);

    /** Read back the PIT status from hardware */
    uint8_t read_back();

    PIT();
    ~PIT() {}
    PIT(PIT&) = delete;
    PIT(PIT&&) = delete;
  };

} //< namespace hw

#endif
