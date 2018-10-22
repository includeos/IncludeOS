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

  class Router_entry {
  public:
    Router_entry(ip6::Addr router, uint16_t lifetime) :
      router_{router}
    {
      update_router_lifetime(lifetime);
    }

    ip6::Addr router() const noexcept
    { return router_; }

    bool expired() const noexcept
    { return RTC::time_since_boot() > invalidation_ts_; }

    void update_router_lifetime(uint16_t lifetime)
    { invalidation_ts_ = RTC::time_since_boot() + lifetime; }

  private:
    ip6::Addr        router_;
    RTC::timestamp_t invalidation_ts_;
  };
}
