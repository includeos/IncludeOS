#pragma once

#include <net/ip6/stateful_addr.hpp>
#include <deque>

namespace net::ndp {
  // Ndp router parameters configured for a particular inet stack
  class Router_params {
  public:
    Router_params() :
      is_router_{false}, send_advertisements_{false},
      managed_flag_{false}, other_flag_{false},
      cur_hop_limit_{255}, link_mtu_{0},
      max_ra_interval_{600}, min_ra_interval_{max_ra_interval_},
      default_lifetime_(3 * max_ra_interval_), reachable_time_{0},
      retrans_time_{0} {}

    bool       is_router_;
    bool       send_advertisements_;
    bool       managed_flag_;
    bool       other_flag_;
    uint8_t    cur_hop_limit_;
    uint16_t   link_mtu_;
    uint16_t   max_ra_interval_;
    uint16_t   min_ra_interval_;
    uint16_t   default_lifetime_;
    uint32_t   reachable_time_;
    uint32_t   retrans_time_;
    std::deque<ip6::Stateful_addr> prefix_list_;
  };
}
