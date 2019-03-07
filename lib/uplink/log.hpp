// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017 Oslo and Akershus University College of Applied Sciences
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
#ifndef UPLINK_LOG_HPP
#define UPLINK_LOG_HPP

#include <util/fixed_vector.hpp>
#include <kernel/events.hpp>
//#include <kernel/os.hpp>
#include <kernel.hpp>
#include <common>

namespace uplink {

/**
 * @brief      A fixed size log which can (should) be hooked up to
 *             OS::stdout - OS::add_stdout({log, &Log::log})
 *
 *             There is a driver for this, see "uplink_log.cpp"
 */
class Log {
public:
  // Log capacity, all data beyond the cap will be discarded.
  static size_t constexpr capacity = 1024*16;

  using Flush_handler = delegate<void(const char*, size_t)>;
  using Internal_log  = Fixed_vector<char, capacity>;

  static Log& get()
  {
    static Log log;
    return log;
  }

  /**
   * @brief      Log text to the internal buffer
   *             and queue an async flush event
   *
   * @param[in]  data  The data
   * @param[in]  len   The length
   */
  void log(const char* data, size_t len)
  {
    if(do_log) // only log & queue if logging enabled
    {
      // make sure we dont exceed the fixed vec buffer
      len = std::min(len, static_cast<size_t>(log_.remaining()));
      if(len > 0)
        log_.insert_replace(log_.end(), data, data+len);

      // if we havent already queued a flush, do it
      // note: OS need to be booted for Events to work
      if(not queued and kernel::is_booted())
        queue_flush();
    }
  }

  /**
   * @brief      Sets the flush handler.
   *             Log will never flush if no handler is set,
   *             but continue buffering.
   *
   * @param[in]  handler  The handler
   */
  void set_flush_handler(Flush_handler handler) noexcept
  { flush_handler = handler; }

  void enable_logging() noexcept
  { do_log = true; }

  void disable_logging() noexcept
  { do_log = false; }

  bool logging_enabled() const noexcept
  { return do_log; }

  /**
   * @brief      Flush the internal buffer if the flush handler is set.
   *             Will also unset queued which makes it available for re-queing.
   *
   *             Logging will be disabled during the flush call, making
   *             all calls to log() being discarded.
   */
  void flush()
  {
    if(flush_handler) // only flush if handler is set
    {
      // due to returning a pointer to the buffer
      // we disable logging so the flush handler
      // dont result in writing into the buffer.
      // it also helps us avoid a never ending recursive loop.
      disable_logging();
      flush_handler(log_.begin(), log_.size());
      log_.clear();
      enable_logging();
    }
    queued = false;
  }

  /**
   * @brief      Queue a async flush.
   */
  void queue_flush()
  {
    if(event_id == 0)
      event_id = Events::get().subscribe({this, &Log::flush});
    Expects(not queued);
    Events::get().trigger_event(event_id);
    queued = true;
  }

private:
  Internal_log  log_;
  Flush_handler flush_handler;
  uint8_t       event_id;
  bool          do_log;
  bool          queued;

  Log()
    : flush_handler{nullptr},
      event_id{0},
      do_log{true}, queued{false}
  {
  }

};

}

#endif
