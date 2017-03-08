// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017 Oslo and Akershus University College of Applied Sciences
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

#ifndef NET_IP4_ICMP4_CODES_HPP
#define NET_IP4_ICMP4_CODES_HPP

namespace net {

  /** ICMP4 Codes so that UDP and IP4 can use these */
  namespace icmp4 {
  namespace code {
    enum class Dest_unreachable : uint8_t {
      NET,
      HOST,
      PROTOCOL,
      PORT,
      FRAGMENTATION,
      SRC_ROUTE,
      NET_UNKNOWN,          // RFC 1122
      HOST_UNKNOWN,
      SRC_HOST_ISOLATED,
      NET_PROHIBITED,
      HOST_PROHIBITED,
      NET_FOR_TOS,
      HOST_FOR_TOS
    };

    enum class Redirect : uint8_t {
      NET,
      HOST,
      TOS_NET,
      TOS_HOST
    };

    enum class Time_exceeded : uint8_t {
      TTL,
      FRAGMENT_REASSEMBLY
    };

    enum class Parameter_problem : uint8_t {
      POINTER_INDICATES_ERROR,
      REQUIRED_OPT_MISSING      // RFC 1122
    };

  } // < namespace code
  } // < namespace icmp4

} // < namespace net

#endif
