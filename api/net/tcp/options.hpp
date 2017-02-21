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
  uint8_t kind;
  uint8_t length;
  uint8_t data[0];

  enum Kind {
    END = 0x00, // End of option list
    NOP = 0x01, // No-Opeartion
    MSS = 0x02, // Maximum Segment Size [RFC 793] Rev: [879, 6691]
    WS  = 0x03, // Window Scaling [RFC 7323] p. 8
  };

  static std::string kind_string(Kind kind) {
    switch(kind) {
    case MSS:
      return {"MSS"};

    case WS:
      return {"WS"};

    default:
      return {"Unknown Option"};
    }
  }

  struct opt_mss {
    uint8_t kind    {MSS};
    uint8_t length  {4};
    uint16_t mss;

    opt_mss(uint16_t mss)
      : mss(htons(mss)) {}

  } __attribute__((packed));

  /**
   * @brief      Window Scaling option [RFC 7323] p. 8
   */
  struct opt_ws {
    uint8_t kind    {WS};
    uint8_t length  {3};
    uint8_t shift_cnt;

    opt_ws(uint8_t shift)
      : shift_cnt{shift} {}

  } __attribute__((packed));

  struct opt_timestamp {
    uint8_t kind;
    uint8_t length;
    uint32_t ts_val;
    uint32_t ts_ecr;
  };

}; // < struct Option

} // < namespace net
} // < namespace tcp

#endif // < NET_TCP_OPTION_HPP
