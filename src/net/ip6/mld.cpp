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
          switch(pckt.type()) {
          case (ICMP_type::MULTICAST_LISTENER_QUERY):
            receive_query(pckt);
            // Change state
            break;
          case (ICMP_type::MULTICAST_LISTENER_REPORT):
            break;
          case (ICMP_type::MULTICAST_LISTENER_DONE):
            break;
          default:
            return;
          }
        };

    state_handlers_[Mld::HostStates::DELAYING_LISTENER] =
        [this] (icmp6::Packet& pckt)
        {
        };
  }

  void Mld::MulticastHostNode::receive_query(icmp6::Packet& pckt)
  {
    auto now_ms = RTC::time_since_boot();
    auto resp_delay = pckt. mld_max_resp_delay();
    auto mcast_addr = pckt.mld_multicast();

    if (mcast_addr != IP6::ADDR_ANY) {
      if (addr() == mcast_addr) {
        if (auto diff = now_ms - timestamp();
            now_ms < timestamp() &&
            resp_delay < timestamp()) {
          update_timestamp(rand() % resp_delay);
        }
      }
    } else {
      if (UNLIKELY(addr() == ip6::Addr::node_all_nodes)) {
        return;
      }
      if (auto diff = now_ms - timestamp();
          now_ms < timestamp() &&
          resp_delay < timestamp()) {
        update_timestamp(rand() % resp_delay);
      }
    }
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

  void Mld::Host::receive(icmp6::Packet& pckt)
  {
    for (auto mcast : mlist_) {
      mcast.handle(pckt);
    }
  }

  void Mld::mcast_expiry()
  {
  }

  void Mld::receive(icmp6::Packet& pckt)
  {
    switch(pckt.type()) {
    case (ICMP_type::MULTICAST_LISTENER_QUERY):
    case (ICMP_type::MULTICAST_LISTENER_REPORT):
    case (ICMP_type::MULTICAST_LISTENER_DONE):
      if (inet_.isRouter()) {
      } else {
          host_.receive(pckt);
      }
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

  Mld2::Mld2(Stack& inet) noexcept:
  inet_ {inet}
  {}

  void Mld2::receive_query(icmp6::Packet& pckt)
  {
    if (!pckt.ip().ip_src().is_linklocal()) {
      PRINT("MLD2: Query does not have linklocal source address\n");
      return;
    }

    auto max_res_code = pckt.mld2_query_max_resp_code();
    auto query = pckt.mld2().query();
    auto mcast = query.multicast;
    auto num_sources = query.num_srcs;

    if (max_res_code < 32768) {
      //max_resp_delay = max_res_code;
    } else {
      //max_resp_delay = ((0x0fff & max_res_code) | 0x1000) <<
      //((0x7000 & max_res_code) + 3);
    }

    if (mcast == IP6::ADDR_ANY) {
      // mcast specific query
    } else {
      // General query
      if (pckt.ip().ip_dst() != ip6::Addr::node_all_nodes) {
        PRINT("MLD2: General Query does not have all nodes multicast "
            "destination address\n");
        return;
      }
    }
  }

  void Mld2::receive_report(icmp6::Packet& pckt)
  {
    auto num_records = pckt.mld2_listner_num_records();
    auto report = pckt.mld2().report();
  }

  void Mld2::receive(icmp6::Packet& pckt)
  {
    switch(pckt.type()) {
    case (ICMP_type::MULTICAST_LISTENER_QUERY):
      break;
    case (ICMP_type::MULTICAST_LISTENER_REPORT_v2):
      break;
    default:
      return;
    }
  }

  bool Mld2::allow(ip6::Addr source_addr, ip6::Addr mcast)
  {
  }

  void Mld2::join(ip6::Addr mcast, FilterMode filtermode, SourceList source_list)
  {
    auto rec = mld_records_.find(mcast);

    if (rec != mld_records_.end()) {
      if (filtermode == FilterMode::EXCLUDE) {
        rec->second.exclude(source_list);
      } else {
        rec->second.include(source_list);
      }
    } else {
      mld_records_.emplace(std::make_pair(mcast,
            Multicast_listening_record{filtermode, source_list}));
    }
  }

} //net
