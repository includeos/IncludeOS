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
