
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

#ifndef NET_IP6_ICMP_ERROR_HPP
#define NET_IP6_ICMP_ERROR_HPP

#include <net/error.hpp>
#include <net/ip6/icmp6_common.hpp>

namespace net {

/**
   *  An object of this error class is sent to UDP and TCP (via Inet) when an ICMP error message
   *  is received in ICMPv6::receive
   */
  class ICMP6_error : public Error {

  public:
    using ICMP_type = icmp6::Type;
    using ICMP_code = uint8_t;

    /**
     * @brief      Constructor
     *             Default: No error occurred
     */
    ICMP6_error()
      : Error{}
    {}

    /**
     * @brief      Constructor
     *
     * @param[in]  icmp_type  The ICMP type
     * @param[in]  icmp_code  The ICMP code
     * @param[in]  pmtu       The Path MTU (Maximum Transmission Unit for the destination)
     *                        This is set in Inet, which asks the IP layer for the most recent
     *                        Path MTU value
     */
    ICMP6_error(ICMP_type icmp_type, ICMP_code icmp_code, uint16_t pmtu = 0)
      : Error{Error::Type::ICMP, "ICMP error message received"},
        icmp_type_{icmp_type}, icmp_code_{icmp_code}, pmtu_{pmtu}
    {}

    ICMP_type icmp_type() const noexcept
    { return icmp_type_; }

    std::string icmp_type_str() const
    { return icmp6::get_type_string(icmp_type_); }

    void set_icmp_type(ICMP_type icmp_type) noexcept
    { icmp_type_ = icmp_type; }

    ICMP_code icmp_code() const noexcept
    { return icmp_code_; }

    std::string icmp_code_str() const
    { return icmp6::get_code_string(icmp_type_, icmp_code_); }

    void set_icmp_code(ICMP_code icmp_code) noexcept
    { icmp_code_ = icmp_code; }

    bool is_too_big() const noexcept {
      return icmp_type_ == ICMP_type::PACKET_TOO_BIG;
    }

    uint16_t pmtu() const noexcept
    { return pmtu_; }

    void set_pmtu(uint16_t pmtu) noexcept
    { pmtu_ = pmtu; }

    std::string to_string() const override
    { return "ICMP " + icmp_type_str() + ": " + icmp_code_str(); }

  private:
    ICMP_type icmp_type_{ICMP_type::NO_ERROR};
    ICMP_code icmp_code_{0};
    uint16_t pmtu_{0};   // Is set if packet sent received an ICMP too big message

  };  // < class ICMP6_error

} //< namespace net

#endif  //< NET_IP6_ICMP_ERROR_HPP
