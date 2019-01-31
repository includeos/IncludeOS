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
//
#pragma once
#ifndef NET_IP6_ICMP6_COMMON_HPP
#define NET_IP6_ICMP6_COMMON_HPP

namespace net {

  /** ICMP6 Codes so that UDP and IP6 can use these */
  namespace icmp6 {

  // ICMP types
  enum class Type : uint8_t {
    DEST_UNREACHABLE              = 1,
    PACKET_TOO_BIG                = 2,
    TIME_EXCEEDED                 = 3,
    PARAMETER_PROBLEM             = 4,
    ECHO                          = 128,
    ECHO_REPLY                    = 129,
    MULTICAST_LISTENER_QUERY      = 130,
    MULTICAST_LISTENER_REPORT     = 131,
    MULTICAST_LISTENER_DONE       = 132,
    ND_ROUTER_SOL                 = 133,
    ND_ROUTER_ADV                 = 134,
    ND_NEIGHBOUR_SOL              = 135,
    ND_NEIGHBOUR_ADV              = 136,
    ND_REDIRECT                   = 137,
    ROUTER_RENUMBERING            = 138,
    INFORMATION_QUERY             = 139,
    INFORMATION_RESPONSE          = 140,
    MULTICAST_LISTENER_REPORT_v2  = 143,
    NO_REPLY                      = 199,  // Custom: Type in ICMP_view if no ping reply received
    NO_ERROR                      = 200
  };
  namespace code {

    enum class Dest_unreachable : uint8_t {
        NO_ROUTE,
        DEST_ADMIN_PROHIBIT,
        NO_SRC_SCOPE,
        ADDR_UNREACHABLE,
        PORT_UNREACHABLE,
        SRC_POLICY_FAIL,
        REJECT_ROUTE_DEST,
        SRC_ROUTING_ERR,
    };

    enum class Time_exceeded : uint8_t {
      HOP_LIMIT,
      FRAGMENT_REASSEMBLY
    };

    enum class Parameter_problem : uint8_t {
      HEADER_ERR,
      UNKNOWN_HEADER,
      UNKNOWN_IPV6_OPT,
    };
  } // < namespace code

  static std::string __attribute__((unused)) get_type_string(Type type) {
    switch (type) {
      case Type::DEST_UNREACHABLE:
        return "DESTINATION UNREACHABLE (1)";
      case Type::PACKET_TOO_BIG:
        return "PACKET TOO BIG (2)";
      case Type::TIME_EXCEEDED:
        return "TIME EXCEEDED (3)";
      case Type::PARAMETER_PROBLEM:
        return "PARAMETER PROBLEM (4)";
      case Type::ECHO:
        return "ECHO (128)";
      case Type::ECHO_REPLY:
        return "ECHO REPLY (129)";
      default:
        return "UNKNOWN";
    }
  }

  static std::string __attribute__((unused)) get_code_string(Type type, uint8_t code) {
    switch (type) {
      case Type::PACKET_TOO_BIG: // Have only code 0
      case Type::ECHO:
      case Type::ECHO_REPLY:
      case Type::MULTICAST_LISTENER_QUERY:
      case Type::MULTICAST_LISTENER_REPORT:
      case Type::MULTICAST_LISTENER_DONE:
      case Type::ND_ROUTER_SOL:
      case Type::ND_ROUTER_ADV:
      case Type::ND_NEIGHBOUR_SOL:
      case Type::ND_NEIGHBOUR_ADV:
      case Type::ND_REDIRECT:
      case Type::ROUTER_RENUMBERING:
      case Type::INFORMATION_QUERY:
      case Type::INFORMATION_RESPONSE:
        return "DEFAULT (0)";
      case Type::DEST_UNREACHABLE:
        switch ( (code::Dest_unreachable) code ) {
          case code::Dest_unreachable::NO_ROUTE:
            return "NO ROUTE(0)";
          case code::Dest_unreachable::DEST_ADMIN_PROHIBIT:
            return "DESTINATION ADMIN PROHIBITED (1)";
          case code::Dest_unreachable::NO_SRC_SCOPE:
            return "SOURCE NO SCOPE(2)";
          case code::Dest_unreachable::ADDR_UNREACHABLE:
            return "ADDR UNREACHABLE(3)";
          case code::Dest_unreachable::PORT_UNREACHABLE:
            return "PORT UNREACHABLE(4)";
          case code::Dest_unreachable::SRC_POLICY_FAIL:
            return "SOURCE POLICY FAILURE(5)";
          case code::Dest_unreachable::REJECT_ROUTE_DEST:
            return "ROUTE DESTINATION REJECTED(6)";
          case code::Dest_unreachable::SRC_ROUTING_ERR:
            return "SOURCE ROUTING ERROR(7)";
          default:
            return "-";
        }
      case Type::TIME_EXCEEDED:
        switch ( (code::Time_exceeded) code ) {
          case code::Time_exceeded::HOP_LIMIT:
            return "HOP LIMIT(0)";
          case code::Time_exceeded::FRAGMENT_REASSEMBLY:
            return "FRAGMENT REASSEMBLY (1)";
          default:
            return "-";
        }
      case Type::PARAMETER_PROBLEM:
        switch ( (code::Parameter_problem) code ) {
          case code::Parameter_problem::HEADER_ERR:
            return "HEADER ERROR (0)";
          case code::Parameter_problem::UNKNOWN_HEADER:
            return "UNKNOWN HEADER (1)";
          case code::Parameter_problem::UNKNOWN_IPV6_OPT:
            return "UNKNOWN IP6 OPTION(2)";
          default:
            return "-";
      }
      default:
        return "-";
    }
  }

  } // < namespace icmp6
} // < namespace net

#endif
