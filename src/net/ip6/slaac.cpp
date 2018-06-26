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

#define SLAAC_DEBUG 1
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

  Slaac::Slaac(Stack& inet)
    : stack(inet),
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
    static bool linklocal_completed = false;

    if (!linklocal_completed) {
      if (dad_retransmits_-- <= 0) {
        // Success. No address collision
        stack.ndp().dad_completed();
        stack.network_config6(tentative_addr_, 64, tentative_addr_ & 64);
        PRINT("Auto-configuring ip6-address %s for stack %s\n",
            tentative_addr_.str().c_str(), stack.ifname().c_str());
        linklocal_completed = true;

        // Start global address autoconfig
        using namespace std::chrono;
        dad_retransmits_ = GLOBAL_RETRIES;
        interval = milliseconds(GLOBAL_INTERVAL);
        timeout_timer_.start(interval);
        auto delay = milliseconds(rand() % (GLOBAL_INTERVAL * 1000));
        timeout_timer_.start(delay);
      } else {
        timeout_timer_.start(interval);
        autoconf_linklocal();
      }
    } else {
      if (dad_retransmits_-- <= 0) {
        // Success. No address collision
        stack.ndp().dad_completed();
        stack.network_config6(tentative_addr_, 64, tentative_addr_ & 64);
        PRINT("Auto-configuring ip6-address %s for stack %s\n",
            tentative_addr_.str().c_str(), stack.ifname().c_str());
        for(auto& handler : this->config_handlers_)
          handler(true);
      } else {
        timeout_timer_.start(interval);
        autoconf_linklocal();
      }
    }
  }

  void Slaac::autoconf_start(int retries, IP6::addr alternate_addr)
  {
    tentative_addr_ = {0xFE80,  0, 0, 0, 0, 0, 0, 0};
    alternate_addr_ = alternate_addr;

    tentative_addr_.set(stack.link_addr());

    // Schedule sending of solicitations for random delay
    using namespace std::chrono;
    this->dad_retransmits_ = retries ? retries : LINKLOCAL_RETRIES;
    this->interval = milliseconds(LINKLOCAL_INTERVAL);
    auto delay = milliseconds(rand() % (LINKLOCAL_INTERVAL * 1000));
    PRINT("Auto-configuring tentative ip6-address %s for %s "
        "with interval:%u and delay:%u ms\n",
           tentative_addr_.str().c_str(), stack.ifname().c_str(),
           interval, delay);
    timeout_timer_.start(delay);
  }

  void Slaac::autoconf_linklocal()
  {
    // Perform DAD
    stack.ndp().perform_dad(tentative_addr_,
      [this] () {
      if(alternate_addr_ != IP6::ADDR_ANY &&
        alternate_addr_ != tentative_addr_) {
        tentative_addr_ = alternate_addr_;
        dad_retransmits_ = 1;
      } else {
        /* DAD has failed. */
        for(auto& handler : this->config_handlers_)
          handler(false);
      }
    });
    /* Try to get a global address */
  }

  void Slaac::autoconf_global()
  {
    // Perform DAD
    stack.ndp().perform_dad(tentative_addr_,
      [this] () {
      if(alternate_addr_ != IP6::ADDR_ANY &&
        alternate_addr_ != tentative_addr_) {
        tentative_addr_ = alternate_addr_;
        dad_retransmits_ = 1;
      } else {
        /* DAD has failed. */
        for(auto& handler : this->config_handlers_)
          handler(false);
      }
    });
    /* Try to get a global address */
  }
}
