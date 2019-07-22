#pragma once

#include <net/ip6/addr.hpp>
#include <rtc>

namespace net::ndp {

  class Router_entry {
  public:
    Router_entry(ip6::Addr router, uint16_t lifetime) :
      router_{router}
    {
      update_router_lifetime(lifetime);
    }

    ip6::Addr router() const noexcept
    { return router_; }

    bool expired() const noexcept
    { return RTC::time_since_boot() > invalidation_ts_; }

    void update_router_lifetime(uint16_t lifetime)
    { invalidation_ts_ = RTC::time_since_boot() + lifetime; }

  private:
    ip6::Addr        router_;
    RTC::timestamp_t invalidation_ts_;
  };
}
