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

#include <net/ip6/stateful_addr.hpp>
#include <deque>

namespace net::ndp {
  // Ndp router parameters configured for a particular inet stack
  class Router_params {
  public:
    Router_params() :
      is_router_{false}, send_advertisements_{false},
      managed_flag_{false}, other_flag_{false},
      cur_hop_limit_{255}, link_mtu_{0},
      max_ra_interval_{600}, min_ra_interval_{max_ra_interval_},
      default_lifetime_(3 * max_ra_interval_), reachable_time_{0},
      retrans_time_{0} {}

    bool       is_router_;
    bool       send_advertisements_;
    bool       managed_flag_;
    bool       other_flag_;
    uint8_t    cur_hop_limit_;
    uint16_t   link_mtu_;
    uint16_t   max_ra_interval_;
    uint16_t   min_ra_interval_;
    uint16_t   default_lifetime_;
    uint32_t   reachable_time_;
    uint32_t   retrans_time_;
    std::deque<ip6::Stateful_addr> prefix_list_;
  };
}
