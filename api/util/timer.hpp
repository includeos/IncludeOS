// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
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

#ifndef UTIL_TIMER_HPP
#define UTIL_TIMER_HPP

#include <timers>

/**
 * @brief A start- and stoppable timer
 * @details A timer which can be started, stopped and restarted.
 * Uses the underlying Timers interface.
 *
 */
class Timer {
public:
  using id_t        = Timers::id_t;
  using duration_t  = Timers::duration_t;
  using handler_t   = delegate<void()>;

  /**
   * @brief Constructs a Timer without a handler
   */
  Timer() : Timer(nullptr) {}

  /**
   * @brief Constructs a Timer with a handler
   *
   * @param on_timeout function to be executed on timeout
   */
  Timer(handler_t on_timeout)
    : id_{Timers::UNUSED_ID},
      on_timeout_{on_timeout}  {}

  /** Move constructor, which stops the other timer and takes over **/
  Timer(Timer&& other)
    : id_{other.id_},
      on_timeout_{std::move(other.on_timeout_)}
  {
    // the other timer is now "stopped"
    other.id_ = Timers::UNUSED_ID;
  }

  /**
   * @brief Start the timer with a timeout duration
   * @details Starts the timer (if not already running)
   * and stores the current timer id returned by the underlying Timers.
   *
   * Requires on_timeout to be set.
   *
   * @param  duration until timing out
   * @param  on_timeout (optional) on timeout handler
   */
  inline void start(duration_t, handler_t on_timeout = nullptr);

  /**
   * @brief Stops the timer
   * @details Stops the timer (if running) by
   * calling stop with the current id to the underlying Timers.
   */
  inline void stop();

  /**
   * @brief Restart the timer
   * @details First stops the timer (if running)
   * and then starts the timer with a new refreshed duration.
   *
   * @param  duration until timing out
   * @param  on_timeout (optional) on timeout handler
   */
  inline void restart(duration_t, handler_t on_timeout = nullptr);

  /**
   * @brief Sets the on timeout handler
   * @details Sets what to be done when the timer times out.
   *
   * @param on_timeout a timeout handler
   */
  void set_on_timeout(handler_t on_timeout)
  { on_timeout_ = on_timeout; }

  /**
   * @brief If the timer is running (active)
   *
   * @return whether the timer is running or not
   */
  bool is_running() const
  { return id_ != Timers::UNUSED_ID; }

  /**
   * @brief Destroys the Timer
   * @details Makes sure to stop any eventual underlying Timer
   * if its running.
   */
  ~Timer()
  { stop(); }

  /** Delete copy and move */
  Timer(const Timer&)             = delete;
  Timer& operator=(const Timer&)  = delete;
  Timer& operator=(Timer&&)       = delete;

private:
  /** ID for the running timer. */
  Timers::id_t id_;
  /** Function to execute on timeout */
  handler_t on_timeout_;

  /**
   * @brief Sets the timer to inactive before calling the user callback
   * @details Wraps the on timeout handler, setting the timer to not running
   * before calling the timeout handler.
   * This is the delegate being sent to the underlying Timers interface
   *
   * @param id the timer id returned by the Timers interface
   */
  inline void _internal_timeout(id_t id);

}; // < class Timer

inline void Timer::start(duration_t when, handler_t on_timeout) {
  if(!is_running())
  {
    if(on_timeout)
      set_on_timeout(on_timeout);

    id_ = Timers::oneshot(when, {this, &Timer::_internal_timeout});
  }
}

inline void Timer::stop() {
  if(is_running())
  {
    Timers::stop(id_);
    id_ = Timers::UNUSED_ID;
  }
}

inline void Timer::restart(duration_t when, handler_t on_timeout) {
  stop();
  start(when, on_timeout);
}

inline void Timer::_internal_timeout(id_t) {
  id_ = Timers::UNUSED_ID;
  if(on_timeout_)
    on_timeout_();
}

#endif
