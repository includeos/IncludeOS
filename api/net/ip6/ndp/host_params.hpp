#pragma once

#include <cmath>
#include <cstdint>

namespace net::ndp {
  static constexpr int    REACHABLE_TIME             = 30000; // in milliseconds
  static constexpr int    RETRANS_TIMER              = 1000;  // in milliseconds
  static constexpr double MIN_RANDOM_FACTOR          = 0.5;
  static constexpr double MAX_RANDOM_FACTOR          = 1.5;

  // Ndp host parameters configured for a particular inet stack
  struct Host_params {
  public:
    Host_params() :
      link_mtu_{1500}, cur_hop_limit_{255},
      base_reachable_time_{REACHABLE_TIME},
      retrans_time_{RETRANS_TIMER} {
        reachable_time_ = compute_reachable_time();
      }

    // Compute random time in the range of min and max
    // random factor times base reachable time
    double compute_reachable_time()
    {
      auto lower = MIN_RANDOM_FACTOR * base_reachable_time_;
      auto upper = MAX_RANDOM_FACTOR * base_reachable_time_;

      return (std::fmod(rand(), (upper - lower + 1)) + lower);
    }

    uint16_t link_mtu_;
    uint8_t  cur_hop_limit_;
    uint32_t base_reachable_time_;
    uint32_t reachable_time_;
    uint32_t retrans_time_;
  };
}
