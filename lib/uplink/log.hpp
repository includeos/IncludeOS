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
#include <common>
#include <os>

namespace uplink {

class Log {
public:
  static size_t constexpr capacity = 1024*8;

  using Flush_handler = delegate<void(const char*, size_t)>;
  using Internal_log  = Fixed_vector<char, capacity>;

  static Log& get()
  {
    static Log log;
    return log;
  }

  void log(const char* data, size_t len)
  {
    if(do_log)
    {
      len = std::min(len, static_cast<size_t>(log_.remaining()));
      log_.insert(log_.end(), data, data+len);

      if(not queued and OS::is_booted())
        queue_flush();
    }
  }

  void set_flush_handler(Flush_handler handler) noexcept
  { flush_handler = handler; }

  void enable_logging() noexcept
  { do_log = true; }

  void disable_logging() noexcept
  { do_log = false; }

  bool logging_enabled() const noexcept
  { return do_log; }

  void flush()
  {
    if(flush_handler)
    {
      disable_logging();
      flush_handler(log_.begin(), log_.size());
      log_.clear();
      enable_logging();
    }
    queued = false;
  }

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

