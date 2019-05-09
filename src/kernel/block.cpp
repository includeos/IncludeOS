// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2016 Oslo and Akershus University College of Applied Sciences
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

#include <os>
#include <statman>
#include <kernel/events.hpp>

// Keep track of blocking levels
static uint32_t* blocking_level = nullptr;
static uint32_t* highest_blocking_level = nullptr;

// Getters, mostly for testing
extern "C" uint32_t os_get_blocking_level() {
  return *blocking_level;
}
extern "C" uint32_t os_get_highest_blocking_level() {
  return *highest_blocking_level;
}

/**
 * A quick and dirty implementation of blocking calls, which simply halts,
 * then calls  the event loop, then returns.
 **/
void os::block() noexcept
{
  // Initialize stats
  if (not blocking_level) {
    blocking_level = &Statman::get()
      .create(Stat::UINT32, std::string("blocking.current_level")).get_uint32();
    *blocking_level = 0;
  }

  if (not highest_blocking_level) {
    highest_blocking_level = &Statman::get()
      .create(Stat::UINT32, std::string("blocking.highest")).get_uint32();
    *highest_blocking_level = 0;
  }

  // Increment level
  *blocking_level += 1;

  // Increment highest if applicable
  if (*blocking_level > *highest_blocking_level)
    *highest_blocking_level = *blocking_level;

  // Process immediate events
  Events::get().process_events();

  // Await next interrupt
  os::halt();

  // Process events (again?)
  Events::get().process_events();

  // Decrement level
  *blocking_level -= 1;
}
