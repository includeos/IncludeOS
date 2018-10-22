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

#include <cmath>
#include <cstdint>

namespace net::ndp {
  static constexpr int    REACHABLE_TIME             = 30000; // in milliseconds
  static constexpr int    RETRANS_TIMER              = 1000;  // in milliseconds
  static constexpr double MIN_RANDOM_FACTOR          = 0.5;
  static constexpr double MAX_RANDOM_FACTOR          = 1.5;

  // Ndp host parameters configured for a particular inet stack
  struct Host_params {
  public:
    Host_params() :
      link_mtu_{1500}, cur_hop_limit_{255},
      base_reachable_time_{REACHABLE_TIME},
      retrans_time_{RETRANS_TIMER} {
        reachable_time_ = compute_reachable_time();
      }

    // Compute random time in the range of min and max
    // random factor times base reachable time
    double compute_reachable_time()
    {
      auto lower = MIN_RANDOM_FACTOR * base_reachable_time_;
      auto upper = MAX_RANDOM_FACTOR * base_reachable_time_;

      return (std::fmod(rand(), (upper - lower + 1)) + lower);
    }

    uint16_t link_mtu_;
    uint8_t  cur_hop_limit_;
    uint32_t base_reachable_time_;
    uint32_t reachable_time_;
    uint32_t retrans_time_;
  };
}
