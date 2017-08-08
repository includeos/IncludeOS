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

#ifndef KERNEL_EVENTS_HPP
#define KERNEL_EVENTS_HPP

#include <delegate>
#include <util/fixed_bitmap.hpp>
#include <smp>

#define IRQ_BASE    32

class alignas(SMP_ALIGN) Events {
public:
  typedef void (*intr_func) ();
  using event_callback = delegate<void()>;

  static const size_t  NUM_EVENTS = 128;

  uint8_t subscribe(event_callback);
  void subscribe(uint8_t evt, event_callback);
  void unsubscribe(uint8_t evt);

  // register event for deferred processing
  void trigger_event(uint8_t evt);

  /**
   * Get per-cpu instance
   */
  static Events& get();
  static Events& get(int cpu);

  /** process all pending events */
  void process_events();

  /** array of received events */
  auto& get_received_array() const noexcept
  { return received_array; }

  /** array of handled events */
  auto& get_handled_array() const noexcept
  { return handled_array; }

  void init_local();
  Events() = default;

private:
  Events(Events&) = delete;
  Events(Events&&) = delete;
  Events& operator=(Events&&) = delete;
  Events& operator=(Events&) = delete;

  event_callback callbacks[NUM_EVENTS];
  std::array<uint64_t, NUM_EVENTS> received_array;
  std::array<uint64_t*,NUM_EVENTS> handled_array;

  Fixed_bitmap<NUM_EVENTS>  event_subs;
  Fixed_bitmap<NUM_EVENTS>  event_pend;
  Fixed_bitmap<NUM_EVENTS>  event_todo;
};

#endif //< KERNEL_EVENTS_HPP
