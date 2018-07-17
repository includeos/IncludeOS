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

//#define MLD_DEBUG 1
#ifdef MLD_DEBUG
#define PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define PRINT(fmt, ...) /* fmt */
#endif
#include <vector>
#include <net/inet>
#include <net/ip6/mld.hpp>
#include <net/ip6/packet_mld.hpp>
#include <net/ip6/icmp6.hpp>
#include <statman>

namespace net
{
  Mld::Mld(Stack& inet) noexcept:
  inet_ {inet}
  {}

  void Mld::Host::expiry()
  {
    for (auto mcast : mlist_) {
      if (mcast.expired()) {
        mld::mld_send_report(mcast.addr());
      }
    }
  }

  void Mld::Host::receive_mcast_query(ip6::Addr mcast_addr, uint16_t resp_delay)
  {
    auto now_ms = RTC::time_since_boot();
    for (auto mcast : mlist_) {
      if (mcast.addr() == mcast_addr) {
        if (auto diff = now_ms - mcast.timestamp();
            now_ms < mcast.timestamp() &&
            resp_delay < mcast.timestamp()) {
          mcast.update_timestamp(rand() % resp_delay);
        }
      }
    }
  }

  void Mld::Host::update_all_resp_time(icmp6::Packet& pckt, uint16_t resp_delay)
  {
    auto now_ms = RTC::time_since_boot();

    for(auto mcast : mlist_) {
      if (UNLIKELY(mcast.addr() == ip6::Addr::node_all_nodes)) {
        continue;
      }
      if (auto diff = now_ms - mcast.timestamp();
          now_ms < mcast.timestamp() &&
          resp_delay < mcast.timestamp()) {
        mcast.update_timestamp(rand() % resp_delay);
      }
    }
  }

  void Mld::mcast_expiry()
  {
  }

  void Mld::receive_query(icmp6::Packet& pckt)
  {
    auto resp_delay = pckt. mld_max_resp_delay();
    auto mcast_addr = pckt.mld_multicast(); 

    if (inet_.isRouter()) {
    } else {

      if (mcast_addr != IP6::ADDR_ANY) {
          host_.receive_mcast_query(mcast_addr, resp_delay);
      } else {
        // General query
        host_.update_all_resp_time(pckt, resp_delay);
      }
    }
  }

  void Mld::receive(icmp6::Packet& pckt)
  {
    switch(pckt.type()) {
    case (ICMP_type::MULTICAST_LISTENER_QUERY):
      break;
    case (ICMP_type::MULTICAST_LISTENER_REPORT):
      break;
    case (ICMP_type::MULTICAST_LISTENER_DONE):
      break;
    case (ICMP_type::MULTICAST_LISTENER_REPORT_v2):
      break;
    default:
      return;
    }
  }
} //net
