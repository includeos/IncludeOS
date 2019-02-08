// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2018 IncludeOS AS, Oslo, Norway
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

#include "header.hpp"
#include <net/iana.hpp>
#include <delegate>

namespace net::ip6 {

  struct Extension_header
  {
    uint8_t  next_header;
    uint8_t  hdr_ext_len;
    uint16_t opt_1;
    uint32_t opt_2;

    Protocol proto() const
    {
      return static_cast<Protocol>(next_header);
    }
    uint8_t size() const
    {
      return sizeof(Extension_header) + hdr_ext_len;
    }
    uint8_t extended() const
    {
      return hdr_ext_len;
    }
  } __attribute__((packed));

  /**
   * @brief      Parse the upper layer protocol (TCP/UDP/ICMPv6).
   *             If none, IPv6_NONXT is returned.
   *
   * @param[in]  start  The start of the extension header
   * @param[in]  proto  The protocol
   *
   * @return     Upper layer protocol
   */
  Protocol parse_upper_layer_proto(const uint8_t* start, const uint8_t* end, Protocol proto);

  using Extension_header_inspector = delegate<void(const Extension_header*)>;
  /**
   * @brief      Iterate and inspect all extension headers
   *
   * @param[in]  start       The start of the extension header
   * @param[in]  proto       The protocol
   * @param[in]  on_ext_hdr  Function to be called on every extension header
   *
   * @return     Number of _bytes_ occupied by extension headers (options)
   */
  uint16_t parse_extension_headers(const Extension_header* start, Protocol proto,
                                   Extension_header_inspector on_ext_hdr);

}
