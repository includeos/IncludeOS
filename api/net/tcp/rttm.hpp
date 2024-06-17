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

#include <expects>
#include <chrono>
#include <cmath>
#include <algorithm>
#include "common.hpp"

namespace net {
namespace tcp {

/** Round Trip Time Measurer, based on Jacobson/ Karels Algorithm found in RFC 793 (rev RFC 6298) */
// TODO: Appendix G.  RTO Calculation Modification https://tools.ietf.org/html/rfc7323#appendix-G
struct RTTM {
  using milliseconds = std::chrono::milliseconds;
  using seconds      = std::chrono::duration<float>; // seconds as float

  // clock granularity
  static constexpr float CLOCK_G {clock_granularity};

  static constexpr float K = 4.0;
  static constexpr float alpha = 1.0 / 8.0;
  static constexpr float beta  = 1.0 / 4.0;

  seconds       SRTT;     // smoothed round-trip time
  seconds       RTTVAR;   // round-trip time variation
  seconds       RTO;      // retransmission timeout

  milliseconds  time;     // tick when measure is started
  uint32_t      samples;  // number of samples made

  /**
   * @brief      Construct a RTTM with default values
   */
  RTTM()
  : SRTT{1.0},
    RTTVAR{1.0},
    RTO{1.0},
    time{0},
    samples{0}
  {}

  /**
   * @brief      Returns whether the RTTM is currently "measuring" (time is set)
   *
   * @return     True if the RTTM is active (measuring)
   */
  bool active() const noexcept
  { return time > milliseconds::zero(); }

  /**
   * @brief      Starts measuring from the current timestamp.
   *             Can only be used if not already active.
   *
   * @param[in]  ts    A timestamp in milliseconds
   */
  void start(milliseconds ts)
  {
    Expects(not active());
    time = ts;
  }

  /**
   * @brief      Stops the measuring, taking a RTT measurment.
   *             Expects the RTTM to be active (a measurment is started).
   *
   * @param[in]  ts    A timestamp in milliseconds
   */
  void stop(milliseconds ts)
  {
    Expects(active());
    rtt_measurement(time - ts);
    time = milliseconds::zero();
  }

  /**
   * @brief      Returns the current calculated RTO (Round trip timeout) in milliseconds
   *
   * @return     The current calculated RTO in milliseconds.
   */
  milliseconds rto_ms() const
  { return std::chrono::duration_cast<milliseconds>(RTO); }

  /**
   * @brief      Calculates the RTO (Round trip time) in seconds.
   *
   * @return     The current RTO.
   */
  // TODO: It would be preferable to remove the RTO member and rather use this function than the getter.
  // Then the RTO would only have to be calculated when a new rtx timer is started,
  // instead of with every single measurement.
  // Two thoughts (see Connection::rtx_timeout):
  // 1. A multiplier member could be added to be used for "backing off" the timer (return compute_rto() * multiplier).
  // 2. Dunno how to set RTO = 3s when timeout on ACK for SYN.
  seconds compute_rto() const
  { return seconds{std::max(SRTT.count() + std::max(CLOCK_G, K * RTTVAR.count()), 1.0f)}; }

  /**
   * @brief      Take a RTT (Round trip time) measurment
   *
   * @param[in]  R     A RTT sample
   */
  void rtt_measurement(milliseconds R);

  /**
   * @brief      Updates the RTO (Round trip time)
   */
  void update_rto()
  { RTO = compute_rto(); }

}; // < struct RTTM

} // < namespace tcp
} // < namespace net

#endif // < NET_TCP_RTTM_HPP
