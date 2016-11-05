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
#ifndef NET_TCP_RTTM_HPP
#define NET_TCP_RTTM_HPP

#include <cassert>
#include <cmath>
#include <debug>

namespace net {
namespace tcp {

// [RFC 6298]
// Round Trip Time Measurer
struct RTTM {
  using timestamp_t = double;
  using duration_t = double;

  // clock granularity
  //static constexpr duration_t CLOCK_G = hw::PIT::frequency().count() / 1000;
  static constexpr duration_t CLOCK_G = 0.0011;

  static constexpr double K = 4.0;

  static constexpr double alpha = 1.0/8;
  static constexpr double beta = 1.0/4;

  timestamp_t t; // tick when measure is started

  duration_t SRTT; // smoothed round-trip time
  duration_t RTTVAR; // round-trip time variation
  duration_t RTO; // retransmission timeout

  bool active;

  RTTM()
    : t(0), SRTT(1.0), RTTVAR(1.0), RTO(1.0), active(false)
  {}

  void start();

  void stop(bool first = false);

  auto rto_ms() const
  { return std::chrono::milliseconds{static_cast<unsigned long>(RTO * 1000)}; }

  /*
    When the first RTT measurement R is made, the host MUST set

    SRTT <- R
    RTTVAR <- R/2
    RTO <- SRTT + max (G, K*RTTVAR)

    where K = 4.
  */
  void first_rtt_measurement(duration_t R) {
    SRTT = R;
    RTTVAR = R/2;
    update_rto();
  }

  /*
    When a subsequent RTT measurement R' is made, a host MUST set

    RTTVAR <- (1 - beta) * RTTVAR + beta * |SRTT - R'|
    SRTT <- (1 - alpha) * SRTT + alpha * R'

    The value of SRTT used in the update to RTTVAR is its value
    before updating SRTT itself using the second assignment.  That
    is, updating RTTVAR and SRTT MUST be computed in the above
    order.

    The above SHOULD be computed using alpha=1/8 and beta=1/4 (as
    suggested in [JK88]).

    After the computation, a host MUST update
    RTO <- SRTT + max (G, K*RTTVAR)
  */
  void sub_rtt_measurement(duration_t R) {
    RTTVAR = (1 - beta) * RTTVAR + beta * std::abs(SRTT-R);
    SRTT = (1 - alpha) * SRTT + alpha * R;
    update_rto();
  }

  void update_rto() {
    RTO = std::max(SRTT + std::max(CLOCK_G, K * RTTVAR), 1.0);
    debug2("<TCP::Connection::RTO> RTO updated: %ums\n",
      (uint32_t)(RTO * 1000));
  }
}; // < struct RTTM

} // < namespace tcp
} // < namespace net

#endif // < NET_TCP_RTTM_HPP
