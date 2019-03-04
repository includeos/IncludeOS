// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2018-2019 Oslo and Akershus University College of Applied Sciences
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

#include <net/ip6/stateful_addr.hpp>
#include <vector>

namespace net::ip6
{
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
              uint32_t pref_lifetime, uint32_t valid_lifetime);

    int input_autoconf(const ip6::Addr& addr, uint8_t prefix,
                       uint32_t pref_lifetime, uint32_t valid_lifetime);

    std::string to_string() const;

  private:
    List list;
  };

}
