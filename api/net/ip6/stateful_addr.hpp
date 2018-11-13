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
#include <rtc>

namespace net::ip6 {
  // Naming...
  class Stateful_addr {
  public:
    // zero means invalid, maybe need to change.
    //static constexpr uint32_t infinite_lifetime = 0xffffffff; // ¯\_(∞)_/¯

    Stateful_addr(ip6::Addr addr, uint8_t prefix, uint32_t preferred_lifetime,
                  uint32_t valid_lifetime)
      : addr_{addr},
        preferred_ts_{preferred_lifetime ?
          (RTC::time_since_boot() + preferred_lifetime) : 0},
        valid_ts_{valid_lifetime ?
          (RTC::time_since_boot() + valid_lifetime) : 0},
        prefix_{prefix}
    {}

    const ip6::Addr& addr() const noexcept
    { return addr_; }

    ip6::Addr& addr() noexcept
    { return addr_; }

    uint8_t prefix() const noexcept
    { return prefix_; }

    bool match(const ip6::Addr& other) const noexcept
    { return (addr_ & prefix_) == (other & prefix_); }

    bool preferred() const noexcept
    { return preferred_ts_ ? RTC::time_since_boot() < preferred_ts_ : true; }

    bool valid() const noexcept
    { return valid_ts_ ? RTC::time_since_boot() > valid_ts_ : true; }

    bool always_valid() const noexcept
    { return valid_ts_; }

    uint32_t remaining_valid_time()
    { return valid_ts_ < RTC::time_since_boot() ? 0 : valid_ts_ - RTC::time_since_boot(); }

    void update_preferred_lifetime(uint32_t preferred_lifetime)
    {
      preferred_ts_ = preferred_lifetime ?
          (RTC::time_since_boot() + preferred_lifetime) : 0;
    }

    void update_valid_lifetime(uint32_t valid_lifetime)
    {
      valid_ts_ = valid_lifetime ?
        (RTC::time_since_boot() + valid_lifetime) : 0;
    }

    auto preferred_ts() const noexcept
    { return preferred_ts_; }

    auto valid_ts() const noexcept
    { return valid_ts_; }

    std::string to_string() const
    { return {addr_.to_string() + "/" + std::to_string(prefix_)}; }

  private:
    ip6::Addr        addr_;
    RTC::timestamp_t preferred_ts_;
    RTC::timestamp_t valid_ts_;
    uint8_t          prefix_;

  };


  class Addr_list {
  public:
    using List = std::vector<Stateful_addr>;

    ip6::Addr get_src(const ip6::Addr& dest) const noexcept
    {
      return (not dest.is_linklocal()) ?
        get_first_unicast() : get_first_linklocal();
    }

    ip6::Addr get_first() const noexcept
    { return (not list.empty()) ? list.front().addr() : ip6::Addr::addr_any; }

    ip6::Addr get_first_unicast() const noexcept
    {
      for(const auto& sa : list)
        if(not sa.addr().is_linklocal()) return sa.addr();
      return ip6::Addr::addr_any;
    }

    ip6::Addr get_first_linklocal() const noexcept
    {
      for(const auto& sa : list)
        if(sa.addr().is_linklocal()) return sa.addr();
      return ip6::Addr::addr_any;
    }

    bool has(const ip6::Addr& addr) const noexcept
    {
      return std::find_if(list.begin(), list.end(),
        [&](const auto& sa){ return sa.addr() == addr; }) != list.end();
    }

    bool empty() const noexcept
    { return list.empty(); }

    List::iterator find(const ip6::Addr& addr)
    {
      return std::find_if(list.begin(), list.end(),
        [&](const auto& sa){ return sa.addr() == addr; });
    }

    int input(const ip6::Addr& addr, uint8_t prefix,
      uint32_t pref_lifetime, uint32_t valid_lifetime)
    {
      auto it = std::find_if(list.begin(), list.end(),
        [&](const auto& sa){ return sa.addr() == addr; });

      if (it == list.end())
      {
        if (valid_lifetime or addr.is_linklocal()) {
          list.emplace_back(addr, prefix, pref_lifetime, valid_lifetime);
          return 1;
        }
      }
      else
      {
        if (valid_lifetime) {
          it->update_valid_lifetime(valid_lifetime);
        }
        else {
          list.erase(it);
          return -1;
        }
      }
      return 0;
    }

    int input_autoconf(const ip6::Addr& addr, uint8_t prefix,
      uint32_t pref_lifetime, uint32_t valid_lifetime)
    {
      auto it = std::find_if(list.begin(), list.end(),
        [&](const auto& sa){ return sa.addr() == addr; });

      if (it == list.end())
      {
        if (valid_lifetime) {
          list.emplace_back(addr, prefix, pref_lifetime, valid_lifetime);
          return 1;
        }
      }
      else if (!it->always_valid())
      {
        it->update_preferred_lifetime(pref_lifetime);
        static constexpr uint32_t two_hours = 60 * 60 * 2;

        if ((valid_lifetime > two_hours) ||
          (valid_lifetime > it->remaining_valid_time()))
        {
          /* Honor the valid lifetime only if its greater than 2 hours
           * or more than the remaining valid time */
          it->update_valid_lifetime(valid_lifetime);
        }
        else if (it->remaining_valid_time() > two_hours)
        {
          it->update_valid_lifetime(two_hours);
        }
      }
      return 0;
    }

    std::string to_string() const
    {
      if(UNLIKELY(list.empty())) return "";
      auto it = list.begin();
      std::string output = it->to_string();
      while(++it != list.end())
        output += "\n" + it->to_string();

      return output;
    }

  private:
    List list;
  };
}
