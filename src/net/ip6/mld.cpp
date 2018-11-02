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
#include <net/ip6/icmp6.hpp>
#include <statman>

namespace net
{
  static const ip6::Addr MLDv2_report_mcast{0xff02,0,0,0,0,0,0,0x0016};

  Mld::Mld(Stack& inet) noexcept
    : inet_{inet},
    host_{*this},
    router_{*this} {}

  Mld::MulticastHostNode::MulticastHostNode()
  {
    state_handlers_[Mld::HostStates::NON_LISTENER] = State_handler{this,
      &Mld::MulticastHostNode::non_listener_state_handler};

    state_handlers_[Mld::HostStates::DELAYING_LISTENER] = State_handler{this,
      &Mld::MulticastHostNode::delay_listener_state_handler};

    state_handlers_[Mld::HostStates::IDLE_LISTENER] = State_handler{this,
      &Mld::MulticastHostNode::idle_listener_state_handler};
  }

  void Mld::MulticastHostNode::non_listener_state_handler(icmp6::Packet& pckt)
  {
  }

  void Mld::MulticastHostNode::delay_listener_state_handler(icmp6::Packet& pckt)
  {
    switch(pckt.type()) {
    case (ICMP_type::MULTICAST_LISTENER_QUERY):
      //receive_query(pckt);
      // Change state
      break;
    case (ICMP_type::MULTICAST_LISTENER_REPORT):
      break;
    case (ICMP_type::MULTICAST_LISTENER_DONE):
      break;
    default:
      return;
    }
  }

  void Mld::MulticastHostNode::idle_listener_state_handler(icmp6::Packet& pckt)
  {
  }


  void Mld::MulticastHostNode::receive_query(const mld::Query& query)
  {
    auto now_ms = RTC::time_since_boot();
    uint32_t resp_delay = query.max_res_delay_ms().count();
    auto mcast_addr = query.mcast_addr;

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
        mld_.send_report(mcast.addr());
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

  void Mld::receive(net::Packet_ptr pkt)
  {
    auto pckt_ip6 = static_unique_ptr_cast<PacketIP6>(std::move(pkt));
    auto pckt = icmp6::Packet(std::move(pckt_ip6));

    PRINT("MLD Receive type: ");
    switch(pckt.type())
    {
      case (ICMP_type::MULTICAST_LISTENER_QUERY):
      {
        const auto len = pckt.ip().payload_length();
        if(len == 24)
        {
          PRINT("Query\n");
          recv_query(pckt);
          break;
        }
        else if(len >= 28)
        {
          PRINT("Query (v2)\n")
          recv_query_v2(pckt);
          break;
        }
        PRINT("Bad sized query\n");
        break;
      }
      case (ICMP_type::MULTICAST_LISTENER_REPORT):
        PRINT("Report\n");
        recv_report(pckt);
        break;
      case (ICMP_type::MULTICAST_LISTENER_DONE):
        PRINT("Done\n");
        recv_done(pckt);
        break;
      case (ICMP_type::MULTICAST_LISTENER_REPORT_v2):
        PRINT("Report (v2)\n");
        recv_report_v2(pckt);
        break;
      default:
        PRINT("Unknown type %#x\n", (uint8_t)pckt.type());
    }
  }

  void Mld::recv_query(icmp6::Packet& pckt)
  {
    const auto& query = pckt.view_payload_as<mld::Query>();

    if(query.is_general())
    {

    }
    else
    {

    }
  }

  void Mld::recv_report(icmp6::Packet& pckt)
  {}

  void Mld::recv_done(icmp6::Packet& pckt)
  {}

  void Mld::recv_query_v2(icmp6::Packet& pckt)
  {}

  void Mld::recv_report_v2(icmp6::Packet& pckt)
  {}

  void Mld::transmit(icmp6::Packet& pckt, MAC::Addr mac)
  {
    linklayer_out_(pckt.release(), mac, Ethertype::IP6);
  }

  void Mld::send_report(const ip6::Addr& mcast)
  {
    icmp6::Packet req(inet_.ip6_packet_factory());

    req.ip().set_ip_src(inet_.ip6_addr());

    req.ip().set_ip_hop_limit(1);
    req.set_type(ICMP_type::MULTICAST_LISTENER_REPORT);
    req.set_code(0);
    req.ip().set_ip_dst(mcast);

    auto& report = req.emplace<mld::Report>(mcast);

    auto dest = req.ip().ip_dst();
    MAC::Addr dest_mac(0x33,0x33,
            dest.get_part<uint8_t>(12),
            dest.get_part<uint8_t>(13),
            dest.get_part<uint8_t>(14),
            dest.get_part<uint8_t>(15));

    req.set_checksum();

    PRINT("MLD: Sending MLD report: %i payload size: %i,"
        "checksum: 0x%x\n, source: %s, dest: %s\n",
        req.ip().size(), req.payload().size(), req.compute_checksum(),
        req.ip().ip_src().str().c_str(),
        req.ip().ip_dst().str().c_str());

    transmit(req, dest_mac);
  }

  void Mld::send_report_v2(const ip6::Addr& mcast)
  {
    icmp6::Packet req(inet_.ip6_packet_factory());

    req.ip().set_ip_src(inet_.ip6_addr());

    req.ip().set_ip_hop_limit(1);
    req.set_type(ICMP_type::MULTICAST_LISTENER_REPORT_v2);
    req.set_code(0);
    req.ip().set_ip_dst(MLDv2_report_mcast);

    auto& report = req.emplace<mld::v2::Report>();
    auto n = report.insert(0, {mld::v2::CHANGE_TO_EXCLUDE, mcast});
    req.ip().increment_data_end(n);

    auto dest = req.ip().ip_dst();
    MAC::Addr dest_mac(0x33,0x33,
            dest.get_part<uint8_t>(12),
            dest.get_part<uint8_t>(13),
            dest.get_part<uint8_t>(14),
            dest.get_part<uint8_t>(15));

    req.set_checksum();

    PRINT("MLD: Sending MLD v2 report: %i payload size: %i,"
        "checksum: 0x%x\n, source: %s, dest: %s\n",
        req.ip().size(), req.payload().size(), req.compute_checksum(),
        req.ip().ip_src().str().c_str(),
        req.ip().ip_dst().str().c_str());

    transmit(req, dest_mac);
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

    const auto& query = pckt.view_payload_as<mld::v2::Query>();

    auto max_res_code = ntohs(query.max_res_code);
    auto mcast = query.mcast_addr;
    auto num_sources = ntohs(query.num_srcs);


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
    const auto& report = pckt.view_payload_as<mld::v2::Report>();
    auto num_records = ntohs(report.num_records);
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
