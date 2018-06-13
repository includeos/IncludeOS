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
#include <net/inet>
#include <net/ip6/slaac.hpp>
#include <statman>

namespace net
{
  const int Slaac::NUM_RETRIES;

  Slaac::Slaac(Stack& inet)
    : stack(inet),
      domain_name{},
      timeout_timer_{{this, &Slaac::restart_auto_config}}
  {
    // default timed out handler spams logs
    this->on_config(
    [this] (bool timed_out)
    {
      if (timed_out)
        MYINFO("Negotiation timed out (%s)", this->stack.ifname().c_str());
      else
        MYINFO("Configuration complete (%s)", this->stack.ifname().c_str());
    });
  }

  void Slaac::on_config(config_func handler)
  {
    assert(handler);
    config_handlers_.push_back(handler);
  }

  void Slaac::restart_auto_config()
  {
    if (retries-- <= 0)
    {
      // give up when retries reached zero
      end_negotiation(true);
    }
    else
    {
      timeout_timer_.start(this->timeout);
      send_first();
    }
  }

  void Slaac::autoconfig(uint32_t timeout_secs)
  {

    MAC::Addr link_addr = stack.link_addr();
    IP6::addr tentative_addr = {0xFE80,  0, 0, 0, 0, 0, 0, 0}; 

    for (int i = 5, int j = 10; i >= 0; i--) {
      tentative_addr.set_part<uint8_t>(j++, link_addr[i]);
    }
    // TODO: Join all-nodes and solicited-node multicast address of the
    // tentaive address

    // Allow multiple calls to negotiate without restarting the process
    this->retries = NUM_RETRIES;
    this->progress = 0;

    // calculate progress timeout
    using namespace std::chrono;
    this->timeout = seconds(timeout_secs) / NUM_RETRIES;

    PRINT("Auto-configuring tentative ip6-address %s for %s\n",
            tentative_addr.str().c_str(), stack.ifname().c_str());

    restart_auto_config();
  }

  void Slaac::send_first()
  {
    if (in_progress_) {
        stack.ndp().send_neighbour_solicitation(tentative_addr);
        stack.ndp().on_neighbour_advertisement(
          [this] (IP6::addr target)
          {
          }
        );
    } else {
        /* Try to get a global address */
    }
  }
}
