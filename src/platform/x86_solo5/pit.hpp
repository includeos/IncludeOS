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
#include <hertz>

namespace x86
{
  /**
   * Programmable Interval Timer
   *
  **/
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
      return MHz(0);
    }

    /** Estimate cpu frequency based on the fixed PIT frequency and rdtsc.
        @Note This is an asynchronous function.  */
    static double estimate_CPU_frequency();

    /** Get the (single) instance. */
    static PIT& get() {
      static PIT instance_;
      return instance_;
    }

  private:

    // Timer handler & expiration timestamp
    timeout_handler           handler;
    timeout_handler           forev_handler;
    std::chrono::milliseconds expiration;

    PIT();
    ~PIT() {}
    PIT(PIT&) = delete;
    PIT(PIT&&) = delete;
  };

} //< namespace hw

#endif
