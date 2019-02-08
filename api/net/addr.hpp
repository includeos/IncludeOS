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
#include <net/ip4/addr.hpp>
#include <net/ip6/addr.hpp>

namespace net {

union Addr {
public:
  static const Addr addr_any;

  constexpr Addr() noexcept
    : ip6_{} {}

  Addr(ip4::Addr addr) noexcept
    : ip4_{0, ip4_sign_be, std::move(addr)} {}

  Addr(ip6::Addr addr) noexcept
    : ip6_{std::move(addr)} {}

  // IP4 variant
  Addr(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
    : ip4_{0, ip4_sign_be, {a, b, c, d}}
  {}

  // IP6 variant
  Addr(uint16_t a, uint16_t b, uint16_t c, uint16_t d,
       uint16_t e, uint16_t f, uint16_t g, uint16_t h)
    : ip6_{a, b, c, d, e, f, g, h}
  {}

  bool is_v4() const noexcept
  { return ip4_.big == 0 and ip4_.sign == ip4_sign_be; }

  bool is_v6() const noexcept
  { return not is_v4(); }

  void set_v4(ip4::Addr addr) noexcept
  {
    ip4_.big  = 0;
    ip4_.sign = ip4_sign_be;
    ip4_.addr = std::move(addr);
  }

  void set_v6(ip6::Addr addr) noexcept
  {
    ip6_ = std::move(addr);
  }

  ip4::Addr v4() const
  {
    if(UNLIKELY(not is_v4()))
      throw ip4::Invalid_address{"Address is not v4"};
    return ip4_.addr;
  }

  const ip6::Addr& v6() const noexcept
  { return ip6_; }

  bool is_any() const noexcept
  { return ip6_ == ip6::Addr::addr_any or (is_v4() and ip4_.addr == 0); }

  Addr any_addr() const noexcept
  { return is_v4() ? Addr{ip4::Addr::addr_any} : Addr{ip6::Addr::addr_any}; }

  std::string to_string() const
  { return is_v4() ? ip4_.addr.to_string() : ip6_.to_string(); }

  Addr(const Addr& other) noexcept
    : ip6_{other.ip6_} {}

  Addr(Addr&& other) noexcept
    : ip6_{std::move(other.ip6_)} {}

  Addr& operator=(const Addr& other) noexcept
  {
    ip6_ = other.ip6_;
    return *this;
  }
  Addr& operator=(Addr&& other) noexcept
  {
    ip6_ = std::move(other.ip6_);
    return *this;
  }

  //operator ip4::Addr() const
  //{ return v4(); }

  //operator const ip6::Addr&() const
  //{ return v6(); }

  bool operator==(const Addr& other) const noexcept
  { return ip6_ == other.ip6_; }

  bool operator!=(const Addr& other) const noexcept
  { return ip6_ != other.ip6_; }

  bool operator<(const Addr& other) const noexcept
  { return ip6_ < other.ip6_; }

  bool operator>(const Addr& other) const noexcept
  { return ip6_ > other.ip6_; }

  bool operator>=(const Addr& other) const noexcept
  { return (*this > other or *this == other); }

  bool operator<=(const Addr& other) const noexcept
  { return (*this < other or *this == other); }

private:
  struct {
    uint64_t  big;
    uint32_t  sign;
    ip4::Addr addr;
  } ip4_;
  ip6::Addr ip6_;

  static constexpr uint32_t ip4_sign_be{0xFFFF0000};
};

static_assert(sizeof(Addr) == sizeof(ip6::Addr));

}
