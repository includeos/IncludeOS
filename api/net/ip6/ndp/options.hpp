// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2018 Oslo and Akershus University College of Applied Sciences
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

#include <net/ip6/addr.hpp>

namespace net::ndp::option {

  enum Type : uint8_t {
    END = 0,
    SOURCE_LL_ADDR = 1, /* RFC2461 */
    TARGET_LL_ADDR = 2, /* RFC2461 */
    PREFIX_INFO = 3,    /* RFC2461 */
    REDIRECT_HDR = 4,   /* RFC2461 */
    MTU = 5,            /* RFC2461 */
    NONCE = 14,         /* RFC7527 */
    ROUTE_INFO = 24,    /* RFC4191 */
    RDNSS = 25,         /* RFC5006 */
    DNSSL = 31,         /* RFC6106 */
    IP6CO = 34            /* RFC6775 */
  };

  /**
 * @brief      General option base. Needs to be inherited by a NDP option.
 */
  struct Base
  {
    const Type    type    {END};
    const uint8_t length  {0};

    /**
     * @brief      Returns the total size of the option (included 2 bytes for code & length)
     *
     * @return     Total size of the option in bytes
     */
    constexpr uint8_t size() const noexcept
    { return length * 8; }

  protected:
    constexpr Base(const Type t, const uint8_t len) noexcept
      : type{t}, length{len}
    {
      Expects(type != Type::END);
      Expects(length != 0);
    }

  } __attribute__((packed));

  /**
   * @brief      Determines what type of option it is. Used for "reflection".
   *
   * @tparam     T     option type
   */
  template <Type T> struct type
  { static constexpr Type TYPE{T}; };

  template <typename Addr>
  struct Source_link_layer_address : public type<SOURCE_LL_ADDR>, public Base
  {
    const Addr addr;

    Source_link_layer_address(Addr linkaddr)
      : Base{type::TYPE, 1}, addr{std::move(linkaddr)}
    {
      static_assert(sizeof(Source_link_layer_address<Addr>) <= 8);
    }
  } __attribute__((packed));

  template <typename Addr>
  struct Target_link_layer_address : public type<TARGET_LL_ADDR>, public Base
  {
    const Addr addr;

    Target_link_layer_address(Addr linkaddr)
      : Base{type::TYPE, 1}, addr{std::move(linkaddr)}
    {
      static_assert(sizeof(Target_link_layer_address<Addr>) <= 8);
    }
  } __attribute__((packed));

  struct Prefix_info : public type<PREFIX_INFO>, public Base
  {
    enum class Flag : uint8_t
    {
      onlink      = 1 << 7,
      autoconf    = 1 << 6, // Autonomous address-configuration
      router_addr = 1 << 5
    };

    uint8_t   prefix_len;
    uint8_t   flag;
    uint32_t  valid;
    uint32_t  preferred;
    uint32_t  reserved2;
    ip6::Addr prefix;

    bool onlink() const noexcept
    { return flag & static_cast<uint8_t>(Flag::onlink); }

    bool autoconf() const noexcept
    { return flag & static_cast<uint8_t>(Flag::autoconf); }

    bool router_addr() const noexcept
    { return flag & static_cast<uint8_t>(Flag::router_addr); }

    constexpr uint32_t valid_lifetime() const noexcept
    { return ntohl(valid); }

    constexpr uint32_t preferred_lifetime() const noexcept
    { return ntohl(preferred); }

    Prefix_info(ip6::Addr addr)
      : Base{type::TYPE, 4}, prefix{std::move(addr)}
    {}

  } __attribute__((packed));
  static_assert(sizeof(Prefix_info) == 4*8);

  struct Mtu : public type<MTU>, public Base
  {
    uint32_t mtu;

    Mtu(const uint32_t mtu)
      : Base{type::TYPE, 1}, mtu{mtu}
    {}

  } __attribute__((packed));
}
