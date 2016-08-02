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
#ifndef KERNEL_TIMER_HPP
#define KERNEL_TIMER_HPP

/**
 * 1. There are no restrictions on when timers can be started or stopped
 * 2. A period of 0 means start a one-shot timer
 * 3. Each timer is a separate object living in a "fixed" vector
 * 4. A dead timer is simply a timer which has its handler reset, as well as
 *     having been removed from schedule
 * 5. No timer may be scheduled more than once at a time, as that will needlessly
 *     inflate the schedule container, as well as complicate stopping timers
 * 6. Free timers are allocated from a stack of free timer IDs (or through expanding
 *     the "fixed" vector)
**/


#include <cstdint>
#include <functional>
#include <delegate>

class Timers
{
public:
  typedef uint32_t id_t;
  typedef uint32_t timestamp_t;
  typedef delegate<void()> handler_t;
  
  /// create a timer that begins @when and repeats every @period
  /// returns a timer id
  static id_t create(timestamp_t when, timestamp_t period, const handler_t&);
  // un-schedule timer, and free it
  static void stop(id_t);
};

class ITimerControl
{
  
};

#endif
