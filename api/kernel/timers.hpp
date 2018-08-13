// -*-C++-*-
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
#ifndef KERNEL_TIMERS_HPP
#define KERNEL_TIMERS_HPP

#include <cstdint>
#include <chrono>
#include <functional>
#include <delegate>

class Timers
{
public:
  using id_t       = int32_t;
  using duration_t = std::chrono::nanoseconds;
  using handler_t  = delegate<void(id_t)>;

  static constexpr id_t UNUSED_ID = -1;

  /// create a one-shot timer that triggers @when from now
  /// returns a timer id
  static id_t oneshot(duration_t when, handler_t);
  /// create a periodic timer that begins @when and repeats every @period
  static id_t periodic(duration_t period, handler_t);
  static id_t periodic(duration_t when, duration_t period, handler_t);
  // un-schedule timer, and free it
  static void stop(id_t);

  /// returns the number of current, active timers
  static size_t active();
  /// returns the number of existing timers
  static size_t existing();
  /// returns the number of free timers
  static size_t free();

  /// returns the time to next timer, or zero
  /// implementations may treat 1 nanosecond as "timer activation imminent"
  static duration_t next();

  /// NOTE: All above operations operate on the current CPU
  /// NOTE: There is a separate timer system on each active CPU

  /// initialization
  typedef delegate<void(duration_t)> start_func_t;
  typedef delegate<void()> stop_func_t;
  static void init(const start_func_t&, const stop_func_t&);

  /// returns true when timers are enabled (globally)
  static bool is_ready();

  /// handler that processes timer interrupts
  static void timers_handler();
  /// signal from the underlying hardware that it is calibrated and ready to go
  static void ready();
};



inline Timers::id_t Timers::oneshot(duration_t when, handler_t handler)
{
  return periodic(when, std::chrono::milliseconds(0), handler);
}

inline Timers::id_t Timers::periodic(duration_t period, handler_t handler)
{
  return periodic(period, period, handler);
}

#endif
