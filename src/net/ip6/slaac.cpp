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
    : stack(inet), alternate_addr_(IP6::ADDR_ANY),
    tentative_addr_({IP6::ADDR_ANY,0,0}), linklocal_completed(false),
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
      stack.add_addr(tentative_addr_.addr(), 0, 0, 64);
      PRINT("Auto-configuring ip6-address %s for stack %s\n",
          tentative_addr_.addr().str().c_str(), stack.ifname().c_str());
      linklocal_completed = true;

      // Start global address autoconfig
      autoconf_global();
    }
    // link local complete, lets do global
    else
    {
      stack.ndp().dad_completed();
      stack.add_addr_autoconf(tentative_addr_.addr(),
        tentative_addr_.preferred_ts(),
        tentative_addr_.valid_ts(),
        64);
      PRINT("Auto-configuring ip6-address %s for stack %s\n",
          tentative_addr_.addr().str().c_str(), stack.ifname().c_str());

      for(auto& handler : this->config_handlers_)
        handler(true);
    }
  }

  void Slaac::autoconf_start(int retries, IP6::addr alternate_addr)
  {
    tentative_addr_ = {ip6::Addr::link_local(stack.link_addr().eui64()), 0, 0};
    alternate_addr_ = alternate_addr;
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
  }

  void Slaac::autoconf_global()
  {
    using namespace std::chrono;
    dad_transmits_ = GLOBAL_RETRIES;
    interval = milliseconds(GLOBAL_INTERVAL*1000);

    stack.ndp().send_router_solicitation({this, &Slaac::process_prefix_info});

    //auto delay = milliseconds(rand() % (GLOBAL_INTERVAL * 1000));
    //timeout_timer_.start(delay);
  }

  void Slaac::perform_dad()
  {
    dad_transmits_--;
    // Perform DAD
    stack.ndp().perform_dad(tentative_addr_.addr(), {this, &Slaac::dad_handler});
    timeout_timer_.start(interval);
  }

  void Slaac::dad_handler([[maybe_unused]]const ip6::Addr& addr)
  {
    if(alternate_addr_ != IP6::ADDR_ANY &&
        alternate_addr_ != tentative_addr_.addr())
    {
      tentative_addr_ = {alternate_addr_, 0, 0};
      dad_transmits_ = 1;
    }
    else {
      timeout_timer_.stop();
      /* DAD has failed. */
      for(auto& handler : this->config_handlers_)
        handler(false);
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
    auto eui64 = MAC::Addr::eui64(stack.link_addr());
    addr.set_part(1, eui64);

    if(not stack.addr6_config().has(addr))
    {
      tentative_addr_ = {addr, preferred_lifetime, valid_lifetime};
      autoconf_trigger();
    }
    else
    {
      stack.add_addr_autoconf(addr, preferred_lifetime, valid_lifetime, prefix_len);
    }
  }

}
