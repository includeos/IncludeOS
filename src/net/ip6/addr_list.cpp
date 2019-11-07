
#include <net/ip6/addr_list.hpp>

namespace net::ip6
{
  int Addr_list::input(const ip6::Addr& addr, uint8_t prefix,
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
        it->update_preferred_lifetime(pref_lifetime);
      }
      else {
        list.erase(it);
        return -1;
      }
    }
    return 0;
  }

  int Addr_list::input_autoconf(const ip6::Addr& addr, uint8_t prefix,
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

  std::string Addr_list::to_string() const
  {
    if(UNLIKELY(list.empty())) return "";
    auto it = list.begin();
    std::string output = it->to_string();
    while(++it != list.end())
      output += "\n" + it->to_string();

    return output;
  }

}
