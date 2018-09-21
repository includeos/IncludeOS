
#pragma once

#include "options.hpp"

namespace net::ndp {

  template <typename M>
  struct View : public M
  {
    using Message = M;
    uint8_t options[0];

    View(Message&& args)
      : Message{std::forward(args)}
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
        ++n;
        on_option(opt);
        raw += opt->size();
        opt = reinterpret_cast<const option::Base*>(raw);
      }

      return n;
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

  } __attribute__((packed));
  static_assert(sizeof(Neighbor_sol) == 20);

  struct Neighbor_adv
  {
    uint32_t  router:1,
              solicited:1,
              override:1,
              reserved:29;
    ip6::Addr target;

  } __attribute__((packed));
  static_assert(sizeof(Neighbor_adv) == 20);
}
