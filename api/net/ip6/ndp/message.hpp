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

#include "options.hpp"

namespace net::ndp {

  template <typename M>
  struct View : public M
  {
    using Message = M;
    uint8_t options[0];

    View() = default;
    View(Message&& args)
      : Message{args}
    {}


    /** Invoked with a const pointer of an "anonymous" option. */
    using Option_inspector = delegate<void(const option::Base*)>;

    uint8_t parse_options(const uint8_t* end, Option_inspector on_option) const
    {
      Expects(on_option);
      uint8_t n = 0;

      const auto* raw = reinterpret_cast<const uint8_t*>(options);
      const auto* opt = reinterpret_cast<const option::Base*>(raw);

      while(opt->type != option::END and raw < end)
      {
        // no options can have zero size
        if (UNLIKELY(opt->size() == 0)) break;
        // can't parse an option we don't have the room to read
        if (UNLIKELY(raw + opt->size() > end)) break;

        ++n;
        on_option(opt);
        raw += opt->size();
        opt = reinterpret_cast<const option::Base*>(raw);
      }

      return n;
    }

    template <typename Opt, typename... Args>
    Opt* add_option(size_t offset, Args&&... args) noexcept
    {
      auto* opt = new (&options[offset]) Opt(std::forward<Args>(args)...);
      return opt;
    }
  };

  struct Router_sol
  {
    const uint32_t reserved{0x0};

  } __attribute__((packed));
  static_assert(sizeof(Router_sol) == 4);

  struct Router_adv
  {
    uint8_t   cur_hop_limit;
    uint8_t   man_addr_conf:1,
              other_conf:1,
              home_agent:1,
              prf:2,
              proxy:1,
              reserved:2;
    uint16_t  router_lifetime;
    uint32_t  reachable_time;
    uint32_t  retrans_time;

  } __attribute__((packed));
  static_assert(sizeof(Router_adv) == 12);

  struct Router_redirect
  {
    ip6::Addr target;
    ip6::Addr dest;

  } __attribute__((packed));

  struct Neighbor_sol
  {
    const uint32_t reserved{0x0};
    ip6::Addr target;

    Neighbor_sol() = default;
    Neighbor_sol(ip6::Addr tar)
      : target{std::move(tar)}
    {}

  } __attribute__((packed));
  static_assert(sizeof(Neighbor_sol) == 20);

  struct Neighbor_adv
  {
    enum Flag : uint8_t
    {
      Router    = 0b1000'0000,
      Solicited = 0b0100'0000,
      Override  = 0b0010'0000
    };

    uint32_t flags{0x0};
    ip6::Addr target;

    Neighbor_adv() = default;
    Neighbor_adv(ip6::Addr tar)
      : target{std::move(tar)}
    {}

    void set_flag(uint8_t flag) noexcept
    { flags |= flag; }

    constexpr bool router() const noexcept
    { return flags & Router; }

    constexpr bool solicited() const noexcept
    { return flags & Solicited; }

    constexpr bool override() const noexcept
    { return flags & Override; }

  } __attribute__((packed));
  static_assert(sizeof(Neighbor_adv) == 20);
}
