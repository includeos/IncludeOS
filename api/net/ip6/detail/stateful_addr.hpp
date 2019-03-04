// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2019 Oslo and Akershus University College of Applied Sciences
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

namespace net::ip6::detail
{
  template <typename T>
  class Stateful_addr
  {
  public:
    using Timestamp = typename T::timestamp_t;
    static constexpr uint32_t infinite_lifetime = 0xFFFF'FFFF; // ¯\_(∞)_/¯

    Stateful_addr(ip6::Addr addr, uint8_t prefix,
                  uint32_t preferred_lifetime = infinite_lifetime,
                  uint32_t valid_lifetime     = infinite_lifetime)
      : addr_{std::move(addr)},
        preferred_ts_{
          preferred_lifetime != infinite_lifetime ?
            (T::time_since_boot() + preferred_lifetime) : 0
        },
        valid_ts_{
          valid_lifetime != infinite_lifetime ?
            (T::time_since_boot() + valid_lifetime) : 0
        },
        prefix_{prefix}
    {}

    const ip6::Addr& addr() const noexcept
    { return addr_; }

    ip6::Addr& addr() noexcept
    { return addr_; }

    uint8_t prefix() const noexcept
    { return prefix_; }

    bool preferred() const noexcept
    {
      if(not valid()) return false;
      return preferred_ts_ ? T::time_since_boot() < preferred_ts_ : true;
    }

    bool valid() const noexcept
    { return valid_ts_ ? T::time_since_boot() < valid_ts_ : true; }

    bool always_valid() const noexcept
    { return valid_ts_ == 0; }

    uint32_t remaining_valid_time()
    {
      if(valid_ts_ == 0) return infinite_lifetime;
      if(valid_ts_ < T::time_since_boot()) return 0;
      return valid_ts_ - T::time_since_boot();
    }

    void update_preferred_lifetime(uint32_t preferred_lifetime)
    {
      preferred_ts_ = preferred_lifetime != infinite_lifetime ?
        (T::time_since_boot() + preferred_lifetime) : 0;
    }

    void update_valid_lifetime(uint32_t valid_lifetime)
    {
      valid_ts_ = valid_lifetime != infinite_lifetime ?
        (T::time_since_boot() + valid_lifetime) : 0;
    }

    auto preferred_ts() const noexcept
    { return preferred_ts_ ? preferred_ts_ : infinite_lifetime; }

    auto valid_ts() const noexcept
    { return valid_ts_ ? valid_ts_ : infinite_lifetime; }

    std::string to_string() const
    { return {addr_.to_string() + "/" + std::to_string(prefix_)}; }

    bool match(const ip6::Addr& other) const noexcept
    { return (addr_ & prefix_) == (other & prefix_); }

  private:
    ip6::Addr   addr_;
    Timestamp   preferred_ts_;
    Timestamp   valid_ts_;
    uint8_t     prefix_;

  };

}

