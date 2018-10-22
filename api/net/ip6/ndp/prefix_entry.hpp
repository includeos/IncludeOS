// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2018 Oslo and Akershus University College of Applied Sciences
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

#include <net/ip6/addr.hpp>
#include <rtc>

namespace net::ndp {

  class Prefix_entry {
  public:
    Prefix_entry(ip6::Addr prefix, uint32_t preferred_lifetime,
          uint32_t valid_lifetime)
        : prefix_{prefix}
    {
      preferred_ts_ = preferred_lifetime ?
          (RTC::time_since_boot() + preferred_lifetime) : 0;
      valid_ts_ = valid_lifetime ?
          (RTC::time_since_boot() + valid_lifetime) : 0;
    }

    ip6::Addr prefix() const noexcept
    { return prefix_; }

    bool preferred() const noexcept
    { return preferred_ts_ ? RTC::time_since_boot() < preferred_ts_ : true; }

    bool valid() const noexcept
    { return valid_ts_ ? RTC::time_since_boot() > valid_ts_ : true; }

    bool always_valid() const noexcept
    { return valid_ts_ ? false : true; }

    uint32_t remaining_valid_time()
    { return valid_ts_ < RTC::time_since_boot() ? 0 : valid_ts_ - RTC::time_since_boot(); }

    void update_preferred_lifetime(uint32_t preferred_lifetime)
    {
      preferred_ts_ = preferred_lifetime ?
          (RTC::time_since_boot() + preferred_lifetime) : 0;
    }

    void update_valid_lifetime(uint32_t valid_lifetime)
    {
      valid_ts_ = valid_lifetime ?
        (RTC::time_since_boot() + valid_lifetime) : 0;
    }
  private:
    ip6::Addr        prefix_;
    RTC::timestamp_t preferred_ts_;
    RTC::timestamp_t valid_ts_;
  };
}
