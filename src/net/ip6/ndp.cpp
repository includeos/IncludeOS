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

//#define NDP_DEBUG 1
#ifdef NDP_DEBUG
#define PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define PRINT(fmt, ...) /* fmt */
#endif
#include <vector>
#include <net/inet>
#include <net/ip6/ndp.hpp>
#include <net/ip6/icmp6.hpp>
#include <statman>

namespace net
{
  Ndp::Ndp(Stack& inet) noexcept:
  requests_rx_    {Statman::get().create(Stat::UINT32, inet.ifname() + ".ndp.requests_rx").get_uint32()},
  requests_tx_    {Statman::get().create(Stat::UINT32, inet.ifname() + ".ndp.requests_tx").get_uint32()},
  replies_rx_     {Statman::get().create(Stat::UINT32, inet.ifname() + ".ndp.replies_rx").get_uint32()},
  replies_tx_     {Statman::get().create(Stat::UINT32, inet.ifname() + ".ndp.replies_tx").get_uint32()},
  inet_           {inet},
  mac_            (inet.link_addr())
  {}

  void Ndp::send_neighbour_advertisement(icmp6::Packet& req)
  {
    icmp6::Packet res(inet_.ip6_packet_factory());
    bool any_src = req.ip().ip_src() == IP6::ADDR_ANY;

    // drop if the packet is too small
    if (res.ip().capacity() < IP6_HEADER_LEN
     + (int) res.header_size() + req.payload().size())
    {
      PRINT("WARNING: Network MTU too small for ICMP response, dropping\n");
      return;
    }

    // Populate response IP header
    res.ip().set_ip_src(inet_.ip6_addr());
    if (any_src) {
        res.ip().set_ip_dst(ip6::Addr::node_all_nodes);
    } else {
        res.ip().set_ip_dst(req.ip().ip_src());
    }
    res.ip().set_ip_hop_limit(255);

    // Populate response ICMP header
    res.set_type(ICMP_type::ND_NEIGHBOUR_ADV);
    res.set_code(0);
    res.ndp().set_neighbour_adv_flag(NEIGH_ADV_SOL | NEIGH_ADV_OVERRIDE);

    // Insert target link address, ICMP6 option header and our mac address
    res.set_payload({req.ndp().neighbour_sol().target().data(), 16 });
    res.ndp().set_ndp_options_header(ndp::ND_OPT_TARGET_LL_ADDR, 0x01);
    res.set_payload({reinterpret_cast<uint8_t*> (&link_mac_addr()), 6});

    // Add checksum
    res.set_checksum();

    PRINT("NDP: Neighbor Adv Response dst: %s size: %i payload size: %i,"
        " checksum: 0x%x\n", res.ip().ip_dst().str().c_str(), res.payload().size(),
        res.ip().ip_dst().str().c_str(), res.compute_checksum());

    auto dest = res.ip().ip_dst();
    transmit(res.release(), dest);
  }

  void Ndp::receive_neighbour_advertisement(icmp6::Packet& req)
  {
    ip6::Addr target = req.ndp().neighbour_adv().target();
    uint8_t *lladdr;

    if (target.is_multicast()) {
        PRINT("NDP: neighbour advertisement target address is multicast\n");
        return;
    }

    if (req.ip().ip_dst().is_multicast() &&
        req.ndp().is_flag_solicited()) {
        PRINT("NDP: neighbour destination address is multicast when"
              " solicit flag is set\n");
        return;
    }


    if (dad_handler_ && target == tentative_addr_) {
      PRINT("NDP: NA: DAD failed. We can't use the %s"
            " address on our interface", target.str().c_str());
      dad_handler_();
      return;
    }

    req.ndp().parse(ICMP_type::ND_NEIGHBOUR_ADV);
    lladdr = req.ndp().get_option_data(ndp::ND_OPT_TARGET_LL_ADDR);

    // For now, just create a cache entry, if one doesn't exist
    cache(target, lladdr, req.ndp().is_flag_solicited() ?
            NeighbourStates::REACHABLE : NeighbourStates::STALE,
             NEIGH_UPDATE_WEAK_OVERRIDE |
            (req.ndp().is_flag_override() ? NEIGH_UPDATE_OVERRIDE : 0) |
             NEIGH_UPDATE_OVERRIDE_ISROUTER |
             (req.ndp().is_flag_router() ? NEIGH_UPDATE_ISROUTER : 0));

    auto waiting = waiting_packets_.find(target);
    if (waiting != waiting_packets_.end()) {
      PRINT("Ndp: Had a packet waiting for this IP. Sending\n");
      transmit(std::move(waiting->second.pckt), target);
      waiting_packets_.erase(waiting);
    }
  }

  void Ndp::send_neighbour_solicitation(ip6::Addr target)
  {
    ip6::Addr dest_ip;
    icmp6::Packet req(inet_.ip6_packet_factory());

    req.ip().set_ip_src(inet_.ip6_addr());

    req.ip().set_ip_hop_limit(255);
    req.set_type(ICMP_type::ND_NEIGHBOUR_SOL);
    req.set_code(0);
    req.set_reserved(0);

    // Solicit destination address. Source address
    // option must be present
    req.ip().set_ip_dst(dest_ip.solicit(target));

    // Set target address
    req.set_payload({target.data(), 16});
    req.ndp().set_ndp_options_header(ndp::ND_OPT_SOURCE_LL_ADDR, 0x01);
    req.set_payload({reinterpret_cast<uint8_t*> (&link_mac_addr()), 6});

    req.set_checksum();

    MAC::Addr dest_mac(0x33,0x33,
            req.ip().ip_dst().get_part<uint8_t>(12),
            req.ip().ip_dst().get_part<uint8_t>(13),
            req.ip().ip_dst().get_part<uint8_t>(14),
            req.ip().ip_dst().get_part<uint8_t>(15));

    PRINT("NDP: Sending Neighbour solicit size: %i payload size: %i,"
        "checksum: 0x%x\n, source: %s, dest: %s, dest mac: %s\n",
        req.ip().size(), req.payload().size(), req.compute_checksum(),
        req.ip().ip_src().str().c_str(),
        req.ip().ip_dst().str().c_str(), dest_mac.str().c_str());

    auto dest = req.ip().ip_dst();
    cache(dest, MAC::EMPTY, NeighbourStates::INCOMPLETE, 0);
    transmit(req.release(), dest, dest_mac);
  }

  void Ndp::receive_neighbour_solicitation(icmp6::Packet& req)
  {
    bool any_src = req.ip().ip_src() == IP6::ADDR_ANY;
    ip6::Addr target = req.ndp().neighbour_sol().target();
    uint8_t *lladdr, *nonce_opt;
    uint64_t nonce = 0;

    PRINT("Receive NDP Neighbor solicitation request. Target addr: %s\n",
            target.str().c_str());

    if (target.is_multicast()) {
        PRINT("NDP: neighbour solictation target address is multicast\n");
        return;
    }

    if (any_src && !req.ip().ip_dst().is_solicit_multicast()) {
        PRINT("NDP: neighbour solictation address is any source "
                "but not solicit destination\n");
        return;
    }
    req.ndp().parse(ICMP_type::ND_NEIGHBOUR_SOL);
    lladdr = req.ndp().get_option_data(ndp::ND_OPT_SOURCE_LL_ADDR);

    if (lladdr) {
        if (any_src) {
            PRINT("NDP: bad any source packet with link layer option\n");
            return;
        }
    }

    nonce_opt = req.ndp().get_option_data(ndp::ND_OPT_NONCE);
    if (nonce_opt) {
        //memcpy(&nonce, nonce_opt, 6);
    }

    bool is_dest_multicast = req.ip().ip_dst().is_multicast();

    if (target != inet_.ip6_addr()) {
      PRINT("NDP: not for us. target=%s us=%s\n", target.to_string().c_str(),
              inet_.ip6_addr().to_string().c_str());
      if (dad_handler_ && target == tentative_addr_) {
        PRINT("NDP: NS: DAD failed. We can't use the %s"
              " address on our interface", target.str().c_str());
        dad_handler_();
        return;
      } else if (!proxy_) {
         return;
      } else if (!proxy_(target)) {
         return;
      }
       PRINT("Responding to neighbour sol as a proxy\n");
    }

    if (any_src) {
        if (lladdr) {
            send_neighbour_advertisement(req);
        }
        return;
    }

    /* Update/Create cache entry for the source address */
    cache(req.ip().ip_src(), lladdr, NeighbourStates::STALE,
         NEIGH_UPDATE_WEAK_OVERRIDE| NEIGH_UPDATE_OVERRIDE);

    send_neighbour_advertisement(req);
  }

  void Ndp::send_router_solicitation(RouterAdv_handler delg)
  {
    ra_handler_ = delg;
    send_router_solicitation();
  }

  void Ndp::send_router_solicitation()
  {
    icmp6::Packet req(inet_.ip6_packet_factory());

    req.ip().set_ip_src(inet_.ip6_addr());
    req.ip().set_ip_dst(ip6::Addr::node_all_routers);

    req.ip().set_ip_hop_limit(255);
    req.set_type(ICMP_type::ND_ROUTER_SOL);
    req.set_code(0);
    req.set_reserved(0);

    req.ndp().set_ndp_options_header(ndp::ND_OPT_SOURCE_LL_ADDR, 0x01);
    req.set_payload({reinterpret_cast<uint8_t*> (&link_mac_addr()), 6});

    // Add checksum
    req.set_checksum();

    PRINT("NDP: Router solicit size: %i payload size: %i, checksum: 0x%x\n",
          req.ip().size(), req.payload().size(), req.compute_checksum());

    transmit(req.release(), req.ip().ip_dst());
  }

  void Ndp::receive_router_solicitation(icmp6::Packet& req)
  {
      uint8_t *lladdr;

      /* Not a router. Drop it */
      if (!inet_.ip6_obj().forward_delg()) {
          return;
      }

      if (req.ip().ip_src() == IP6::ADDR_ANY) {
          PRINT("NDP: RS: Source address is any\n");
          return;
      }

      req.ndp().parse(ICMP_type::ND_ROUTER_SOL);
      lladdr = req.ndp().get_option_data(ndp::ND_OPT_SOURCE_LL_ADDR);

      cache(req.ip().ip_src(), lladdr, NeighbourStates::STALE,
         NEIGH_UPDATE_WEAK_OVERRIDE| NEIGH_UPDATE_OVERRIDE |
         NEIGH_UPDATE_OVERRIDE_ISROUTER);
  }

  void Ndp::receive_router_advertisement(icmp6::Packet& req)
  {
      if (!req.ip().ip_src().is_linklocal()) {
        PRINT("NDP: Router advertisement source address is not link-local\n");
        return;
      }

      /* Forwarding is enabled. Does that mean
       * we are a router? We need to consume if we are */
      if (inet_.ip6_obj().forward_delg()) {
          PRINT("NDP: RA: Forwarding is enabled. Not accepting"
                " router advertisement\n");
          return;
      }
      req.ndp().parse(ICMP_type::ND_ROUTER_ADV);
      req.ndp().parse_prefix([this] (ip6::Addr prefix)
      {
        /* Called if autoconfig option is set */
        /* Append mac addres to get a valid address */
        prefix.set(this->inet_.link_addr());
        if (ra_handler_) {
        }
      }, [] (ip6::Addr prefix)
      {
        /* Called if onlink is set */
      });
  }

  void Ndp::receive(icmp6::Packet& pckt)
  {
    switch(pckt.type()) {
    case (ICMP_type::ND_ROUTER_SOL):
      PRINT("NDP: Router solictation message from %s\n", pckt.ip().ip_src().str().c_str());
      receive_router_solicitation(pckt);
      break;
    case (ICMP_type::ND_ROUTER_ADV):
      PRINT("NDP: Router advertisement message from %s\n", pckt.ip().ip_src().str().c_str());
      receive_router_advertisement(pckt);
      break;
    case (ICMP_type::ND_NEIGHBOUR_SOL):
      PRINT("NDP: Neigbor solictation message from %s\n", pckt.ip().ip_src().str().c_str());
      receive_neighbour_solicitation(pckt);
      break;
    case (ICMP_type::ND_NEIGHBOUR_ADV):
      PRINT("NDP: Neigbor advertisement message from %s\n", pckt.ip().ip_src().str().c_str());
      receive_neighbour_advertisement(pckt);
      break;
    case (ICMP_type::ND_REDIRECT):
      PRINT("NDP: Neigbor redirect message from %s\n", pckt.ip().ip_src().str().c_str());
      break;
    default:
      return;
    }
  }

  bool Ndp::lookup(ip6::Addr ip)
  {
      auto entry = neighbour_cache_.find(ip);
      if (entry != neighbour_cache_.end()) {
          return true;
      }
      return false;
  }

  void Ndp::cache(ip6::Addr ip, uint8_t *ll_addr, NeighbourStates state, uint32_t flags)
  {
      if (ll_addr) {
        MAC::Addr mac(ll_addr);
        cache(ip, mac, state, flags);
      }
  }

  void Ndp::cache(ip6::Addr ip, MAC::Addr mac, NeighbourStates state, uint32_t flags)
  {
      PRINT("Ndp Caching IP %s for %s\n", ip.str().c_str(), mac.str().c_str());
      auto entry = neighbour_cache_.find(ip);
      if (entry != neighbour_cache_.end()) {
          PRINT("Cached entry found: %s recorded @ %zu. Updating timestamp\n",
             entry->second.mac().str().c_str(), entry->second.timestamp());
          if (entry->second.mac() != mac) {
            neighbour_cache_.erase(entry);
            neighbour_cache_.emplace(
               std::make_pair(ip, Cache_entry{mac, state, flags})); // Insert
          } else {
            entry->second.set_state(state);
            entry->second.set_flags(flags);
            entry->second.update();
          }
      } else {
          neighbour_cache_.emplace(
            std::make_pair(ip, Cache_entry{mac, state, flags})); // Insert
          if (UNLIKELY(not flush_timer_.is_running())) {
            flush_timer_.start(flush_interval_);
          }
      }
  }

  void Ndp::resolve_waiting()
  {
    PRINT("<ndp> resolve timer doing sweep\n");

    for (auto it =waiting_packets_.begin(); it != waiting_packets_.end();){
      if (it->second.tries_remaining--) {
        ndp_resolver_(it->first);
        it++;
      } else {
        // TODO: According to RFC,
        // Send ICMP destination unreachable
        it = waiting_packets_.erase(it);
      }
    }

    if (not waiting_packets_.empty())
      resolve_timer_.start(1s);

  }

  void Ndp::await_resolution(Packet_ptr pckt, ip6::Addr next_hop)
  {
    auto queue =  waiting_packets_.find(next_hop);
    PRINT("<NDP await> Waiting for resolution of %s\n", next_hop.str().c_str());
    if (queue != waiting_packets_.end()) {
      PRINT("\t * Packets already queueing for this IP\n");
      queue->second.pckt->chain(std::move(pckt));
    } else {
      PRINT("\t *This is the first packet going to that IP\n");
      waiting_packets_.emplace(std::make_pair(next_hop, Queue_entry{std::move(pckt)}));

      // Try resolution immediately
      ndp_resolver_(next_hop);

      // Retry later
      resolve_timer_.start(1s);
    }
  }

  void Ndp::flush_expired()
  {
    PRINT("NDP: Flushing expired entries\n");
    std::vector<ip6::Addr> expired;
    for (auto ent : neighbour_cache_) {
      if (ent.second.expired()) {
        expired.push_back(ent.first);
      }
    }

    for (auto ip : expired) {
      neighbour_cache_.erase(ip);
    }

    if (not neighbour_cache_.empty()) {
      flush_timer_.start(flush_interval_);
    }
  }

  void Ndp::ndp_resolve(ip6::Addr next_hop)
  {
    PRINT("<NDP RESOLVE> %s\n", next_hop.str().c_str());

    // Stat increment requests sent
    requests_tx_++;

    // Send ndp solicit
    send_neighbour_solicitation(next_hop);
  }

  void Ndp::transmit(Packet_ptr pckt, ip6::Addr next_hop, MAC::Addr mac)
  {

    Expects(pckt->size());

    if (mac == MAC::EMPTY) {
        // If we don't have a cached IP, perform NDP sol
        auto neighbour_cache_entry = neighbour_cache_.find(next_hop);
        if (UNLIKELY(neighbour_cache_entry == neighbour_cache_.end())) {
            PRINT("NDP: No cache entry for IP %s.  Resolving. \n", next_hop.to_string().c_str());
            await_resolution(std::move(pckt), next_hop);
            return;
        }

        // Get MAC from cache
        mac = neighbour_cache_[next_hop].mac();

        PRINT("NDP: Found cache entry for IP %s -> %s \n",
            next_hop.to_string().c_str(), mac.to_string().c_str());
    }

    PRINT("<NDP -> physical> Transmitting %u bytes to %s\n",
        (uint32_t) pckt->size(), next_hop.str().c_str());

    // Move chain to linklayer
    linklayer_out_(std::move(pckt), mac, Ethertype::IP6);
  }

  /* Perform Duplicate Address Detection for the specifed address.
   * DAD must be performed on all unicast addresses prior to
   * assigning them to an interface. regardless of whether they
   * are obtained through stateless autoconfiguration,
   * DHCPv6, or manual configuration */
  void Ndp::perform_dad(ip6::Addr tentative_addr,
          Dad_handler delg)
  {
    tentative_addr_ = tentative_addr;
    dad_handler_ = delg;

    // TODO: Join all-nodes and solicited-node multicast address of the
    // tentaive address
    send_neighbour_solicitation(tentative_addr);
  }

  void Ndp::dad_completed()
  {
    dad_handler_ = nullptr;
    tentative_addr_ = IP6::ADDR_ANY;
  }

  // NDP packet function definitions
} // net
