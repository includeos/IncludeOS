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
    END = 0x00, // End of option list
    NOP = 0x01, // No-Opeartion
    MSS = 0x02, // Maximum Segment Size [RFC 793] Rev: [879, 6691]
    WS  = 0x03, // Window Scaling [RFC 7323] p. 8
    TS  = 0x08, // Timestamp [RFC 7323] p. 11
  };

  const uint8_t kind    {END};
  const uint8_t length  {0};
  uint8_t               data[0];

  static std::string kind_string(Kind kind) {
    switch(kind) {
    case MSS:
      return {"MSS"};

    case WS:
      return {"Window Scaling"};

    case TS:
      return {"Timestamp"};

    default:
      return {"Unknown Option"};
    }
  }

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

  struct opt_ts {
    const uint8_t   kind    {TS};
    const uint8_t   length  {10};
    const uint32_t  val;
    const uint32_t  ecr;

    opt_ts(const uint32_t val, const uint32_t echo)
      : val{htonl(val)},
        ecr{htonl(echo)}
    {}

  } __attribute__((packed));

}; // < struct Option

} // < namespace net
} // < namespace tcp

#endif // < NET_TCP_OPTION_HPP
