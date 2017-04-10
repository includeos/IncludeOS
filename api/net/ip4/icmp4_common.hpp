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

#pragma once
#ifndef NET_IP4_ICMP4_COMMON_HPP
#define NET_IP4_ICMP4_COMMON_HPP

namespace net {

  /** ICMP4 Codes so that UDP and IP4 can use these */
  namespace icmp4 {

  // ICMP types
  enum class Type : uint8_t {
    ECHO_REPLY,
    DEST_UNREACHABLE    = 3,
    REDIRECT            = 5,
    ECHO                = 8,
    TIME_EXCEEDED       = 11,
    PARAMETER_PROBLEM   = 12,
    TIMESTAMP           = 13,
    TIMESTAMP_REPLY     = 14,
    NO_REPLY            = 100,  // Custom: Type in ICMP_view if no ping reply received
    NO_ERROR            = 200
  };

  namespace code {

    enum class Dest_unreachable : uint8_t {
      NET,
      HOST,
      PROTOCOL,
      PORT,
      FRAGMENTATION_NEEDED,
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

  static std::string __attribute__((unused)) get_type_string(Type type) {
    switch (type) {
      case Type::ECHO:
        return "ECHO (8)";
      case Type::ECHO_REPLY:
        return "ECHO REPLY (0)";
      case Type::DEST_UNREACHABLE:
        return "DESTINATION UNREACHABLE (3)";
      case Type::REDIRECT:
        return "REDIRECT (5)";
      case Type::TIME_EXCEEDED:
        return "TIME EXCEEDED (11)";
      case Type::PARAMETER_PROBLEM:
        return "PARAMETER PROBLEM (12)";
      case Type::TIMESTAMP:
        return "TIMESTAMP (13)";
      case Type::TIMESTAMP_REPLY:
        return "TIMESTAMP REPLY (14)";
      case Type::NO_REPLY:
        return "NO REPLY";
      default:
        return "UNKNOWN";
    }
  }

  static std::string __attribute__((unused)) get_code_string(Type type, uint8_t code) {
    switch (type) {
      case Type::ECHO:  // Have only code 0
      case Type::ECHO_REPLY:
      case Type::TIMESTAMP:
      case Type::TIMESTAMP_REPLY:
      case Type::NO_REPLY:
        return "DEFAULT (0)";
      case Type::DEST_UNREACHABLE:
        switch ( (code::Dest_unreachable) code ) {
          case code::Dest_unreachable::NET:
            return "NET (0)";
          case code::Dest_unreachable::HOST:
            return "HOST (1)";
          case code::Dest_unreachable::PROTOCOL:
            return "PROTOCOL (2)";
          case code::Dest_unreachable::PORT:
            return "PORT (3)";
          case code::Dest_unreachable::FRAGMENTATION_NEEDED:
            return "FRAGMENTATION NEEDED (4)";
          case code::Dest_unreachable::SRC_ROUTE:
            return "SOURCE ROUTE (5)";
          case code::Dest_unreachable::NET_UNKNOWN:
            return "NET UNKNOWN (6)";
          case code::Dest_unreachable::HOST_UNKNOWN:
            return "HOST UNKNOWN (7)";
          case code::Dest_unreachable::SRC_HOST_ISOLATED:
            return "SOURCE HOST ISOLATED (8)";
          case code::Dest_unreachable::NET_PROHIBITED:
            return "NET PROHIBITED (9)";
          case code::Dest_unreachable::HOST_PROHIBITED:
            return "HOST PROHIBITED (10)";
          case code::Dest_unreachable::NET_FOR_TOS:
            return "NET FOR Type-of-Service (11)";
          case code::Dest_unreachable::HOST_FOR_TOS:
            return "HOST FOR Type-of-Service (12)";
          default:
            return "-";
        }
      case Type::REDIRECT:
        switch ( (code::Redirect) code ) {
          case code::Redirect::NET:
            return "NET (0)";
          case code::Redirect::HOST:
            return "HOST (1)";
          case code::Redirect::TOS_NET:
            return "Type-of-Service NET (2)";
          case code::Redirect::TOS_HOST:
            return "Type-of-Service HOST (3)";
          default:
            return "-";
        }
      case Type::TIME_EXCEEDED:
        switch ( (code::Time_exceeded) code ) {
          case code::Time_exceeded::TTL:
            return "TTL (0)";
          case code::Time_exceeded::FRAGMENT_REASSEMBLY:
            return "FRAGMENT REASSEMBLY (1)";
          default:
            return "-";
        }
      case Type::PARAMETER_PROBLEM:
        switch ( (code::Parameter_problem) code ) {
          case code::Parameter_problem::POINTER_INDICATES_ERROR:
            return "POINTER INDICATES ERROR (0)";
          case code::Parameter_problem::REQUIRED_OPT_MISSING:
            return "REQUIRED OPTION MISSING (1)";
          default:
            return "-";
        }
      default:
        return "-";
    };
  }

  } // < namespace icmp4

} // < namespace net

#endif
