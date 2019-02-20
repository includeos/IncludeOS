// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
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

//#define SLAAC_DEBUG 1
#ifdef SLAAC_DEBUG
#define PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define PRINT(fmt, ...) /* fmt */
#endif
#include <vector>
#include <cstdlib>
#include <net/inet>
#include <net/ip6/slaac.hpp>
#include <statman>

#include <info>
#define MYINFO(X,...) INFO("SLAAC",X,##__VA_ARGS__)

namespace net
{
  const int Slaac::LINKLOCAL_RETRIES;
  const int Slaac::LINKLOCAL_INTERVAL;
  const int Slaac::GLOBAL_RETRIES;
  const int Slaac::GLOBAL_INTERVAL;

  Slaac::Slaac(Stack& inet)
    : stack(inet), token_{0}, use_token_{false},
    tentative_addr_({IP6::ADDR_ANY,64,0,0}), linklocal_completed(false),
    dad_transmits_(LINKLOCAL_RETRIES),
    timeout_timer_{{this, &Slaac::autoconf_trigger}}
  {
    // default timed out handler spams logs
    this->on_config(
    [this] (bool completed)
    {
      if (completed) {
        INFO("SLAAC", "Autoconf completed for (%s)",
            this->stack.ifname().c_str());
      } else {
        INFO("SLAAC", "Autoconf failed for (%s)",
            this->stack.ifname().c_str());
      }
    });
  }

  void Slaac::on_config(config_func handler)
  {
    assert(handler);
    config_handlers_.push_back(handler);
  }

  void Slaac::autoconf_trigger()
  {
    if(dad_transmits_ > 0)
    {
      perform_dad();
      return;
    }

    // Success. No address collision
    // we're out of transmits, and timer has kicked in,
    // which means dad handler hasnt been called and noone
    // has replied having our address

    if (!linklocal_completed)
    {
      stack.ndp().dad_completed();
      stack.add_addr(tentative_addr_.addr(), 64, 0, 0);
      PRINT("Auto-configuring ip6-address %s for stack %s\n",
          tentative_addr_.addr().str().c_str(), stack.ifname().c_str());
      linklocal_completed = true;

      // Start global address autoconfig
      autoconf_global_start();
    }
    // link local complete, lets do global
    else
    {
      stack.ndp().dad_completed();
      stack.add_addr_autoconf(tentative_addr_.addr(), 64,
        tentative_addr_.preferred_ts(),
        tentative_addr_.valid_ts());
      PRINT("Auto-configuring ip6-address %s for stack %s\n",
          tentative_addr_.addr().str().c_str(), stack.ifname().c_str());

      for(auto& handler : this->config_handlers_)
        handler(true);
    }
  }

  void Slaac::autoconf_start(int retries, uint64_t token, bool use_token)
  {
    token_ = token;
    if(use_token)
    {
      Expects(token_ != 0);
      use_token_ = use_token;
      tentative_addr_ = {ip6::Addr::link_local(token_), 64, 0, 0};
    }
    else
    {
      tentative_addr_ = {ip6::Addr::link_local(stack.link_addr().eui64()), 64, 0, 0};
    }

    this->dad_transmits_ = retries;

    autoconf_linklocal();
  }

  void Slaac::autoconf_linklocal()
  {
    // Schedule sending of solicitations for random delay
    using namespace std::chrono;
    this->interval = milliseconds(LINKLOCAL_INTERVAL*1000);
    auto delay = milliseconds(rand() % (LINKLOCAL_INTERVAL * 1000));
    PRINT("Auto-configuring tentative ip6-address %s for %s "
        "with interval:%u and delay:%u ms\n",
           tentative_addr_.addr().str().c_str(), stack.ifname().c_str(),
           interval, delay);
    timeout_timer_.start(delay);

    // join multicast group ?
    //stack.mld().send_report_v2(ip6::Addr::solicit(tentative_addr_.addr()));
  }

  void Slaac::autoconf_global_start()
  {
    dad_transmits_ = GLOBAL_RETRIES;

    autoconf_global();
  }

  void Slaac::autoconf_global()
  {
    // share dad_transmits for this use case as well
    if(dad_transmits_ == 0)
    {
      PRINT("<SLAAC> Out of transmits for router sol (no reply)\n");
      for(auto& handler : this->config_handlers_)
        handler(true); // we do have linklocal at least
      return;
    }
    dad_transmits_--;
    using namespace std::chrono;
    PRINT("<SLAAC> Sending router sol\n");
    stack.ndp().send_router_solicitation({this, &Slaac::process_prefix_info});

    interval = milliseconds(GLOBAL_INTERVAL*1000);
    //auto delay = milliseconds(rand() % (GLOBAL_INTERVAL * 1000));
    timeout_timer_.start(interval, {this, &Slaac::autoconf_global});
  }

  void Slaac::perform_dad()
  {
    dad_transmits_--;
    // Perform DAD
    stack.ndp().perform_dad(tentative_addr_.addr(), {this, &Slaac::dad_handler});
    timeout_timer_.start(interval, {this, &Slaac::autoconf_trigger});
  }

  void Slaac::dad_handler([[maybe_unused]]const ip6::Addr& addr)
  {
    if(token_ and tentative_addr_.addr().get_part<uint64_t>(1) != token_)
    {
      tentative_addr_.addr().set_part(1, token_);
      PRINT("<SLAAC> DAD fail, using supplied token: %zu => %s\n",
        token_, tentative_addr_.addr().to_string().c_str());
      dad_transmits_ = 1;
    }
    else {
      timeout_timer_.stop();
      /* DAD has failed. */
      for(auto& handler : this->config_handlers_)
        handler(linklocal_completed ? true : false);
    }
  }

  // RFC 4862, 5.5.3
  void Slaac::process_prefix_info(const ndp::option::Prefix_info& pinfo)
  {
    Expects(pinfo.autoconf());

    if (UNLIKELY(pinfo.prefix.is_linklocal()))
    {
      PRINT("<SLAAC> Prefix info address is link-local\n");
      return;
    }

    const auto preferred_lifetime = pinfo.preferred_lifetime();
    const auto valid_lifetime     = pinfo.valid_lifetime();

    if (UNLIKELY(preferred_lifetime > valid_lifetime))
    {
      PRINT("<SLAAC> Prefix option has invalid lifetime\n");
      return;
    }

    if (UNLIKELY(pinfo.prefix.is_multicast()))
    {
      PRINT("<SLAAC> Prefix info address is multicast\n");
      return;
    }

    // not sure if correct but should work
    static constexpr uint8_t valid_prefix_len = 64;
    const auto prefix_len = pinfo.prefix_len;
    if (prefix_len != valid_prefix_len)
    {
      PRINT("<SLAAC> Invalid prefix length %u (valid=%u)\n",
        prefix_len, valid_prefix_len);
      return;
    }

    auto addr = pinfo.prefix;
    auto eui64 = (use_token_) ? token_ : MAC::Addr::eui64(stack.link_addr());
    addr.set_part(1, eui64);

    if(not stack.addr6_config().has(addr))
    {
      // this means we're already working with this prefix.
      // there is still a problem when we receive more than one prefix
      // (a different one)
      if(tentative_addr_.addr() == addr)
        return;

      tentative_addr_ = {addr, prefix_len, preferred_lifetime, valid_lifetime};
      dad_transmits_ = GLOBAL_RETRIES;
      timeout_timer_.stop();
      PRINT("<SLAAC> New prefix info, DAD address %s\n", addr.to_string().c_str());
      autoconf_trigger();
    }
    else
    {
      stack.add_addr_autoconf(addr, prefix_len, preferred_lifetime, valid_lifetime);
    }
  }

}
