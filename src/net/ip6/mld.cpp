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
  inet_{inet},
  host_{*this},
  router_{*this}
  {}

  Mld::MulticastHostNode::MulticastHostNode()
  {
    state_handlers_[Mld::HostStates::NON_LISTENER] = 
        [this] (icmp6::Packet& pckt)
        {
        };

    state_handlers_[Mld::HostStates::DELAYING_LISTENER] =
        [this] (icmp6::Packet& pckt)
        {
        };

    state_handlers_[Mld::HostStates::DELAYING_LISTENER] =
        [this] (icmp6::Packet& pckt)
        {
        };
  }

  Mld::MulticastRouterNode::MulticastRouterNode()
  {
    state_handlers_[Mld::RouterStates::QUERIER] = 
        [this] (icmp6::Packet& pckt)
        {
        };

    state_handlers_[Mld::RouterStates::NON_QUERIER] =
        [this] (icmp6::Packet& pckt)
        {
        };
  }

  void Mld::Host::expiry()
  {
    for (auto mcast : mlist_) {
      if (mcast.expired()) {
        mld_.mld_send_report(mcast.addr());
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

  void Mld::mld_send_report(ip6::Addr mcast)
  {
    icmp6::Packet req(inet_.ip6_packet_factory());

    req.ip().set_ip_src(inet_.ip6_addr());

    req.ip().set_ip_hop_limit(1);
    req.set_type(ICMP_type::MULTICAST_LISTENER_REPORT);
    req.set_code(0);
    req.set_reserved(0);

    req.ip().set_ip_dst(ip6::Addr::node_all_routers);

    // Set target address
    req.add_payload(mcast.data(), IP6_ADDR_BYTES);
    req.set_checksum();

    PRINT("MLD: Sending MLD report: %i payload size: %i,"
        "checksum: 0x%x\n, source: %s, dest: %s\n",
        req.ip().size(), req.payload().size(), req.compute_checksum(),
        req.ip().ip_src().str().c_str(),
        req.ip().ip_dst().str().c_str());
  }
} //net
