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
  static id_t start(timestamp_t when, timestamp_t period, const handler_t&);
  // un-schedule timer, and free it
  static void stop(id_t);
};

#endif
