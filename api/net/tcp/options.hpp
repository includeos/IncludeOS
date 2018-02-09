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
#ifndef NET_TCP_OPTION_HPP
#define NET_TCP_OPTION_HPP

#include <net/util.hpp> // byte ordering

namespace net {
namespace tcp {

/*
  TCP Header Option
*/
struct Option {

  enum Kind {
    END       = 0x00, // End of option list
    NOP       = 0x01, // No-Operation
    MSS       = 0x02, // Maximum Segment Size [RFC 793] Rev: [879, 6691]
    WS        = 0x03, // Window Scaling [RFC 7323] p. 8
    SACK_PERM = 0x04, // Sack-Permitted [RFC 2018]
    SACK      = 0x05, // Selective ACK [RFC 2018]
    TS        = 0x08, // Timestamp [RFC 7323] p. 11
  };

  const uint8_t kind    {END};
  const uint8_t length  {0};
  uint8_t               data[0];

  std::string kind_string() const
  { return kind_string(static_cast<Kind>(kind)); }

  static std::string kind_string(Kind kind) {
    switch(kind)
    {
      case MSS: return {"MSS"};
      case WS: return {"Window Scaling"};
      case SACK_PERM: return {"SACK Permitted"};
      case SACK: return {"SACK"};
      case TS: return {"Timestamp"};
      case NOP: return {"No-Operation"};
      case END: return {"End of list"};
      default: return {"Unknown Option"};
    }
  }

  /**
   * @brief      Maximum Segment Size option [RFC 793]
   */
  struct opt_mss {
    const uint8_t   kind    {MSS};
    const uint8_t   length  {4};
    const uint16_t  mss;

    opt_mss(const uint16_t mss)
      : mss(htons(mss)) {}

  } __attribute__((packed));

  /**
   * @brief      Window Scaling option [RFC 7323] p. 8
   */
  struct opt_ws {
    const uint8_t kind    {WS};
    const uint8_t length  {3};
    const uint8_t shift_cnt;

    opt_ws(const uint8_t shift)
      : shift_cnt{shift} {}

  } __attribute__((packed));

  /**
   * @brief      Timestamp option [RFC 7323] p. 11
   */
  struct opt_ts {
    const uint8_t   kind    {TS};
    const uint8_t   length  {10};
    const uint32_t  val;
    const uint32_t  ecr;

    opt_ts(const uint32_t val, const uint32_t echo)
      : val{htonl(val)},
        ecr{htonl(echo)}
    {}

    uint32_t get_val() const noexcept
    { return ntohl(val); }

    uint32_t get_ecr() const noexcept
    { return ntohl(ecr); }

  } __attribute__((packed));

  /**
   * @brief      An aligned version of a Timestamp option. Includes padding,
   *             making the ts values on word boundaries.
   *
      The following layout is recommended for sending options on
      non-<SYN> segments to achieve maximum feasible alignment of 32-bit
      and 64-bit machines.

                   +--------+--------+--------+--------+
                   |   NOP  |  NOP   |  TSopt |   10   |
                   +--------+--------+--------+--------+
                   |          TSval timestamp          |
                   +--------+--------+--------+--------+
                   |          TSecr timestamp          |
                   +--------+--------+--------+--------+
   */
  struct opt_ts_align {
    const uint8_t padding[2] {NOP, NOP};
    const opt_ts  ts;

    opt_ts_align(const uint32_t val, const uint32_t echo)
      : ts{val, echo}
    {}

    uint8_t size() const noexcept
    { return sizeof(padding) + sizeof(ts); }

  } __attribute__((packed));

  /**
   * @brief      SACK Permitted [RFC 2018] p. 11
   */
  struct opt_sack_perm {
    const uint8_t   kind    {SACK_PERM};
    const uint8_t   length  {2};

    opt_sack_perm() = default;

  } __attribute__((packed));

  /**
   * @brief      Timestamp option [RFC 7323] p. 11
   */
  struct opt_sack {
    const uint8_t   kind{SACK};
    const uint8_t   length;
    uint8_t         val[0];

    template <typename Collection>
    opt_sack(const Collection& blocks)
      : length{static_cast<uint8_t>(sizeof(kind) + sizeof(length) +
          (blocks.size() * sizeof(typename Collection::value_type)))}
    {
      using T = typename Collection::value_type;
      Expects(blocks.size() * sizeof(T) <= 4*8);
      std::memcpy(&val[0], blocks.data(), blocks.size() * sizeof(T));
    }

  } __attribute__((packed));

  struct opt_sack_align {
    const uint8_t padding[2] {NOP, NOP};
    const opt_sack  sack;

    template <typename Collection>
    opt_sack_align(const Collection& blocks)
      : sack{blocks}
    {}

    uint8_t size() const noexcept
    { return sizeof(padding) + sack.length; }

  } __attribute__((packed));

}; // < struct Option

} // < namespace net
} // < namespace tcp

#endif // < NET_TCP_OPTION_HPP
