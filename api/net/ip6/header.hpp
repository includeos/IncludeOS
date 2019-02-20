// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2017 Oslo and Akershus University College of Applied Sciences
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
#ifndef NET_IP6_HEADER_HPP
#define NET_IP6_HEADER_HPP

#include <net/ip6/addr.hpp>
#define  IP6_HEADER_LEN 40
#define  IP6_ADDR_BYTES 16

namespace net::ip6 {

/**
 * This type is used to represent the standard IPv6 header
 */
struct Header {
  union {
    uint32_t ver_tc_fl = 0x0060;
    uint32_t version : 4,
             traffic_class : 8,
             flow_label : 20;
  };
  uint16_t payload_length = 0;
  uint8_t  next_header = 0;
  uint8_t  hop_limit   = 0;
  Addr     saddr;
  Addr     daddr;
} __attribute__((packed)); //< struct Header

static_assert(sizeof(Header) == 40, "IPv6 Header is of constant size (40 bytes)");

} //< namespace net::ip6

#endif //< NET_IP6_HEADER_HPP
