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

#define NDP_DEBUG 1
#ifdef NDP_DEBUG
#define PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define PRINT(fmt, ...) /* fmt */
#endif
#include <vector>
#include <net/inet>
#include <net/ip6/ndp.hpp>
#include <net/ip6/icmp6.hpp>
#include <net/ip6/ndp/message.hpp>
#include <statman>

namespace net
{
  Ndp::Ndp(Stack& inet) noexcept:
  requests_rx_    {Statman::get().create(Stat::UINT32, inet.ifname() + ".ndp.requests_rx").get_uint32()},
  requests_tx_    {Statman::get().create(Stat::UINT32, inet.ifname() + ".ndp.requests_tx").get_uint32()},
  replies_rx_     {Statman::get().create(Stat::UINT32, inet.ifname() + ".ndp.replies_rx").get_uint32()},
  replies_tx_     {Statman::get().create(Stat::UINT32, inet.ifname() + ".ndp.replies_tx").get_uint32()},
  inet_           {inet},
  host_params_    {},
  router_params_  {},
  mac_            (inet.link_addr())
  {
    /* Since NDP is present in inet. We need to do ndp interface
     * initialisation here.
     * 1. Join the all node multicast address
     * 2. Join the solicited multicast address for all the ip address
     *    assignged to the interface.
     * 3. We add/remove the interface ip addresses to/from the prefix list.
     * 4. Need to join/leave the multicast solicit address using MLD. */
  }

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

    auto payload  = res.payload();
    auto* data    = payload.data();
    auto& adv     = *reinterpret_cast<ndp::View<ndp::Neighbor_adv>*>(data);

    adv.set_flag(ndp::Neighbor_adv::Solicited | ndp::Neighbor_adv::Override);

    // Insert target link address, ICMP6 option header and our mac address
    const auto& sol = *reinterpret_cast<const ndp::View<ndp::Neighbor_sol>*>(req.payload().data());
    adv.target = sol.target;

    res.ip().increment_data_end(sizeof(sol));
    using Target_ll_addr = ndp::option::Target_link_layer_address<MAC::Addr>;
    auto* opt = adv.add_option<Target_ll_addr>(0, link_mac_addr());
    res.ip().increment_data_end(opt->size());

    // Add checksum
    res.set_checksum();

    PRINT("NDP: Neighbor Adv Response dst: %s payload size: %li checksum: 0x%x\n",
      res.ip().ip_dst().str().c_str(), res.payload().size(), res.compute_checksum());

    auto dest = res.ip().ip_dst();
    transmit(res.release(), dest);
  }

  void Ndp::receive_neighbour_advertisement(icmp6::Packet& req)
  {
    auto payload    = req.payload();
    auto* data      = payload.data();
    const auto& adv = *reinterpret_cast<const ndp::View<ndp::Neighbor_adv>*>(data);

    const auto& target = adv.target;

    if (target.is_multicast()) {
      PRINT("NDP: neighbour advertisement target address is multicast\n");
      return;
    }

    if (req.ip().ip_dst().is_multicast() && adv.solicited()) {
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

    // Parse the options
    adv.parse_options(data + payload.length(), [&](const auto* opt)
    {
      using namespace ndp::option;
      switch(opt->type)
      {
        case TARGET_LL_ADDR:
        {
          const auto lladdr =
            reinterpret_cast<const Target_link_layer_address<MAC::Addr>*>(opt)->addr;

          // For now, just create a cache entry, if one doesn't exist
          cache(target, lladdr,
            adv.solicited() ? NeighbourStates::REACHABLE : NeighbourStates::STALE,
            NEIGH_UPDATE_WEAK_OVERRIDE |
            (adv.override() ? NEIGH_UPDATE_OVERRIDE : 0) |
            NEIGH_UPDATE_OVERRIDE_ISROUTER |
            (adv.router() ? NEIGH_UPDATE_ISROUTER : 0));

          break;
        }
        default:
        {
          // Ignore other options
        }
      }
    });

    auto waiting = waiting_packets_.find(target);
    if (waiting != waiting_packets_.end()) {
      PRINT("Ndp: Had a packet waiting for this IP. Sending\n");
      transmit(std::move(waiting->second.pckt), target);
      waiting_packets_.erase(waiting);
    }
  }

  void Ndp::send_neighbour_solicitation(ip6::Addr target)
  {
    using namespace ndp;
    ip6::Addr dest_ip;

    icmp6::Packet req(inet_.ip6_packet_factory());
    req.ip().set_ip_src(inet_.ip6_addr());

    req.ip().set_ip_hop_limit(255);
    req.set_type(ICMP_type::ND_NEIGHBOUR_SOL);
    req.set_code(0);

    // Solicit destination address. Source address
    // option must be present
    req.ip().set_ip_dst(dest_ip.solicit(target));

    // Construct neigbor sol msg on with target address on our ICMP
    auto& sol = req.emplace<View<Neighbor_sol>>(target);
    static_assert(sizeof(sol) == sizeof(Neighbor_sol));
    /* RFC 4861 p.22
       MUST NOT be included when the source IP address is the
       unspecified address.  Otherwise, on link layers
       that have addresses this option MUST be included in
       multicast solicitations and SHOULD be included in
       unicast solicitations.
    */
    if(req.ip().ip_src() != IP6::ADDR_ANY)
    {
      using Source_ll_addr = ndp::option::Source_link_layer_address<MAC::Addr>;
      auto* opt = sol.add_option<Source_ll_addr>(0, link_mac_addr());
      req.ip().increment_data_end(opt->size());
    }

    req.set_checksum();
    auto dest = req.ip().ip_dst();
    MAC::Addr dest_mac(0x33,0x33,
            dest.get_part<uint8_t>(12),
            dest.get_part<uint8_t>(13),
            dest.get_part<uint8_t>(14),
            dest.get_part<uint8_t>(15));

    PRINT("NDP: Sending Neighbour solicit size: %i payload size: %i,"
        "checksum: 0x%x, src: %s, dst: %s, dmac: %s\n",
        req.ip().size(), req.payload().size(), req.compute_checksum(),
        req.ip().ip_src().str().c_str(),
        req.ip().ip_dst().str().c_str(), dest_mac.str().c_str());

    cache(dest, MAC::EMPTY, NeighbourStates::INCOMPLETE, 0);
    transmit(req.release(), dest, dest_mac);
  }

  void Ndp::receive_neighbour_solicitation(icmp6::Packet& req)
  {
    bool any_src = req.ip().ip_src() == IP6::ADDR_ANY;

    auto payload    = req.payload();
    auto* data      = payload.data();
    const auto& sol = *reinterpret_cast<const ndp::View<ndp::Neighbor_sol>*>(data);

    auto target = sol.target;
    [[maybe_unused]]uint64_t nonce = 0;

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

    MAC::Addr lladdr;
    // Parse the options
    sol.parse_options(data + payload.length(), [&](const auto* opt)
    {
      using namespace ndp::option;
      switch(opt->type)
      {
        case SOURCE_LL_ADDR:
        {
          using Source_ll_addr = Source_link_layer_address<MAC::Addr>;
          lladdr = reinterpret_cast<const Source_ll_addr*>(opt)->addr;
          break;
        }
        case NONCE:
        {
          break;
        }
        default:
        {
          // Ignore other options
        }
      }
    });

    if (lladdr != MAC::EMPTY && any_src) {
      PRINT("NDP: bad any source packet with link layer option\n");
      return;
    }

    bool is_dest_multicast = req.ip().ip_dst().is_multicast();

    // TODO: Change this. Can be targeted to many ip6 address on this inet
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
      if (lladdr != MAC::EMPTY) {
        send_neighbour_advertisement(req);
      }
      return;
    }

    /* Update/Create cache entry for the source address */
    cache(req.ip().ip_src(), lladdr, NeighbourStates::STALE,
         NEIGH_UPDATE_WEAK_OVERRIDE| NEIGH_UPDATE_OVERRIDE);

    send_neighbour_advertisement(req);
  }

  void Ndp::receive_redirect(icmp6::Packet& req)
  {
    /*
    auto dest = req.ndp().router_redirect().dest();
    auto target = req.ndp().router_redirect().target();

    if (!req.ip().ip_src().is_linklocal()) {
      PRINT("NDP: Router Redirect source address is not link-local\n");
      return;
    }

    if (req.ip().hop_limit() != 255) {
      PRINT("NDP: Router Redirect source hop limit is not 255\n");
      return;
    }

    if (req.code() != 0) {
      PRINT("NDP: Router Redirect code is not 0\n");
      return;
    }

    if (dest.is_multicast()) {
      PRINT("NDP: Router Redirect destination is multicast\n");
      return;
    }

    if (!req.ndp().router_redirect().target().is_linklocal() &&
          target != dest) {
      PRINT("NDP: Router Redirect target is not linklocal and is not"
          " equal to destination address\n");
      return;
    }
    req.ndp().parse_options(ICMP_type::ND_REDIRECT);
    dest_cache(dest, target);

   auto lladdr = req.ndp().get_option_data(ndp::ND_OPT_TARGET_LL_ADDR);

   if (lladdr) {
     cache(target, lladdr, NeighbourStates::STALE,
         NEIGH_UPDATE_WEAK_OVERRIDE | NEIGH_UPDATE_OVERRIDE |
         (target == dest) ? 0 :
         (NEIGH_UPDATE_OVERRIDE_ISROUTER| NEIGH_UPDATE_ISROUTER), false);
   }*/
  }

  void Ndp::send_router_solicitation(RouterAdv_handler delg)
  {
    ra_handler_ = delg;
    send_router_solicitation();
  }

  void Ndp::send_router_solicitation()
  {
    using namespace ndp;
    icmp6::Packet req(inet_.ip6_packet_factory());

    req.ip().set_ip_src(inet_.ip6_addr());
    req.ip().set_ip_dst(ip6::Addr::link_all_routers);

    req.ip().set_ip_hop_limit(255);
    req.set_type(ICMP_type::ND_ROUTER_SOL);
    req.set_code(0);


    // Construct router sol msg on with target address on our ICMP
    auto& sol = req.emplace<View<Router_sol>>();
    static_assert(sizeof(sol) == sizeof(Router_sol));

    using Source_ll_addr = option::Source_link_layer_address<MAC::Addr>;
    auto* opt = sol.add_option<Source_ll_addr>(0, link_mac_addr());
    req.ip().increment_data_end(opt->size());

    // Add checksum
    req.set_checksum();

    auto dst = req.ip().ip_dst();
    MAC::Addr dest_mac(0x33,0x33,
        dst.get_part<uint8_t>(12),
        dst.get_part<uint8_t>(13),
        dst.get_part<uint8_t>(14),
        dst.get_part<uint8_t>(15));

    PRINT("NDP: Router solicit size: %i payload size: %i, checksum: 0x%x\n",
          req.ip().size(), req.payload().size(), req.compute_checksum());
    transmit(req.release(), dst, dest_mac);
  }

  void Ndp::receive_router_solicitation(icmp6::Packet& req)
  {
    /* Not a router. Drop it */
    if (!inet_.ip6_obj().forward_delg()) {
      return;
    }

    if (req.ip().ip_src() == IP6::ADDR_ANY) {
      PRINT("NDP: RS: Source address is any\n");
      return;
    }

    auto payload    = req.payload();
    auto* data      = payload.data();
    const auto& sol = *reinterpret_cast<const ndp::View<ndp::Router_sol>*>(data);

    sol.parse_options(data + payload.size(), [&](const auto* opt)
    {
      using namespace ndp::option;
      switch(opt->type)
      {
        case SOURCE_LL_ADDR:
        {
          using Source_ll_addr = Source_link_layer_address<MAC::Addr>;
          const auto lladdr = reinterpret_cast<const Source_ll_addr*>(opt)->addr;

          cache(req.ip().ip_src(), lladdr, NeighbourStates::STALE,
            NEIGH_UPDATE_WEAK_OVERRIDE| NEIGH_UPDATE_OVERRIDE |
            NEIGH_UPDATE_OVERRIDE_ISROUTER);

          break;
        }
        default:
        {
          // Ignore other options
        }
      }
    });
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

      auto payload = req.payload();
      auto* data   = payload.data();
      const auto& adv = *reinterpret_cast<const ndp::View<ndp::Router_adv>*>(data);

      /* Add this router to the router list */
      add_router(req.ip().ip_src(), ntohs(adv.router_lifetime));

      /* TODO: Check if this is a router or a host.
       * Lets assume we are a host for now. Set host params */
      auto reachable_time = adv.reachable_time;
      auto retrans_time   = adv.retrans_time;
      auto cur_hop_limit  = adv.cur_hop_limit;

      if (reachable_time and reachable_time != host().base_reachable_time_)
      {
        host().base_reachable_time_ = reachable_time;
        host().compute_reachable_time();
      }

      if (retrans_time and retrans_time != host().retrans_time_)
      {
        host().retrans_time_ = retrans_time;
      }

      if (cur_hop_limit)
      {
        host().cur_hop_limit_ = cur_hop_limit;
      }

      // Parse the options
      adv.parse_options(data + payload.length(), [&](const auto* opt)
      {
        using namespace ndp::option;
        switch(opt->type)
        {
          case PREFIX_INFO:
          {
            const auto* pinfo = reinterpret_cast<const Prefix_info*>(opt);
            PRINT("NDP: Prefix: %s length=%u\n", pinfo->prefix.to_string().c_str(), pinfo->prefix_len);

            if (pinfo->prefix.is_linklocal())
            {
              PRINT("NDP: Prefix info address is linklocal\n");
              return;
            }

            if (pinfo->onlink())
            {
              PRINT("on link\n");
              //onlink_cb(confaddr, pinfo->prefered, pinfo->valid);
            }

            if (pinfo->autoconf())
            {
              PRINT("autoconf\n");
              if (pinfo->prefix.is_multicast())
              {
                PRINT("NDP: Prefix info address is multicast\n");
                return;
              }

              const auto preferred_lifetime = pinfo->preferred_lifetime();
              const auto valid_lifetime     = pinfo->valid_lifetime();

              if (preferred_lifetime > valid_lifetime)
              {
                PRINT("NDP: Prefix option has invalid lifetime\n");
                return;
              }

              if (pinfo->prefix_len == 64)
              {
                PRINT("prefix_len is 64\n");
                //confaddr.set_part<uint64_t>(1, pinfo->prefix.get_part<uint64_t>(1));
              }
              else
              {
                PRINT("NDP: Prefix option: autoconf: "
                         " prefix with wrong len: %d", pinfo->prefix_len);
                return;
              }

              auto eui64 = MAC::Addr::eui64(inet_.link_addr());
              auto addr = pinfo->prefix;
              addr.set_part(1, eui64);
              add_addr_autoconf(addr, preferred_lifetime, valid_lifetime);

              if(ra_handler_)
                ra_handler_(addr);
            }

            break;
          }

          case SOURCE_LL_ADDR:
          {
            const auto lladdr =
              reinterpret_cast<const Source_link_layer_address<MAC::Addr>*>(opt)->addr;

            cache(req.ip().ip_src(), lladdr, NeighbourStates::STALE,
              NEIGH_UPDATE_WEAK_OVERRIDE| NEIGH_UPDATE_OVERRIDE |
              NEIGH_UPDATE_OVERRIDE_ISROUTER | NEIGH_UPDATE_ISROUTER);

            break;
          }

          case MTU:
          {
            const auto mtu = reinterpret_cast<const Mtu*>(opt)->mtu;

            if (mtu < 1500 && mtu > host().link_mtu_) {
              host().link_mtu_ = mtu;
            }

            break;
          }

          default:
          {
            // Ignore options not allowed in the Router Adv scope
          }
        }
        // do opt
      });

      /*req.ndp().parse_prefix([this] (ip6::Addr prefix,
            uint32_t preferred_lifetime, uint32_t valid_lifetime)
      {
        // Called if autoconfig option is set
        // Append mac addres to get a valid address
        prefix.set(this->inet_.link_addr());
        add_addr_autoconf(prefix, preferred_lifetime, valid_lifetime);
        PRINT("NDP: RA: Adding address %s with preferred lifetime: %u"
            " and valid lifetime: %u\n", prefix.str().c_str(),
            preferred_lifetime, valid_lifetime);

        if (ra_handler_) {
          ra_handler_(prefix);
        }
      }, [this] (ip6::Addr prefix, uint32_t preferred_lifetime,
          uint32_t valid_lifetime)
      {
        //Called if onlink is set
      });*/
  }

  void Ndp::receive(net::Packet_ptr pkt)
  {
    auto pckt_ip6 = static_unique_ptr_cast<PacketIP6>(std::move(pkt));
    auto pckt = icmp6::Packet(std::move(pckt_ip6));

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
      receive_redirect(pckt);
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

  void Ndp::cache(ip6::Addr ip, uint8_t *ll_addr, NeighbourStates state, uint32_t flags, bool update)
  {
    if (ll_addr) {
      MAC::Addr mac(ll_addr);
      cache(ip, mac, state, flags, update);
    }
  }

  void Ndp::cache(ip6::Addr ip, MAC::Addr mac, NeighbourStates state, uint32_t flags, bool update)
  {
    PRINT("Ndp Caching IP %s for %s\n", ip.str().c_str(), mac.str().c_str());
    auto entry = neighbour_cache_.find(ip);
    if (entry != neighbour_cache_.end()) {
      PRINT("Cached entry found: %s recorded @ %zu. Updating timestamp\n",
         entry->second.mac().str().c_str(), entry->second.timestamp());
      if (entry->second.mac() != mac) {
        neighbour_cache_.erase(entry);
        neighbour_cache_.emplace(
           std::make_pair(ip, Neighbour_Cache_entry{mac, state, flags})); // Insert
      } else if (update) {
        entry->second.set_state(state);
        entry->second.set_flags(flags);
        entry->second.update();
      }
    } else {
      neighbour_cache_.emplace(
        std::make_pair(ip, Neighbour_Cache_entry{mac, state, flags})); // Insert
      if (UNLIKELY(not flush_neighbour_timer_.is_running())) {
        flush_neighbour_timer_.start(flush_interval_);
      }
    }
  }

  void Ndp::dest_cache(ip6::Addr dest_ip, ip6::Addr next_hop)
  {
    auto entry = dest_cache_.find(dest_ip);
    if (entry != dest_cache_.end()) {
      entry->second.update(next_hop);
    } else {
      dest_cache_.emplace(std::make_pair(dest_ip,
            Destination_Cache_entry{next_hop}));
    }
  }

  void Ndp::resolve_waiting()
  {
    PRINT("<ndp> resolve timer doing sweep\n");

    for (auto it = waiting_packets_.begin(); it != waiting_packets_.end();) {
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

  void Ndp::check_neighbour_reachability()
  {
  }

  void Ndp::delete_dest_entry(ip6::Addr ip)
  {
    //TODO: Better to have a list of destination
    // list entries inside router entries for faster cleanup
    for(auto it = dest_cache_.begin(); it != dest_cache_.end(); )
    {
      if(it->second.next_hop() == ip)
        it = dest_cache_.erase(it);
      else
        it++;
    }
  }

  void Ndp::flush_expired_routers()
  {
    PRINT("NDP: Flushing expired routers\n");
    // TODO: Check the head of the router list.
    // If that isn't expired. None of them after it is
    for (auto ent = router_list_.begin(); ent != router_list_.end();) {
      if (!ent->expired()) {
        delete_dest_entry(ent->router());
        ent = router_list_.erase(ent);
      } else {
        ent++;
      }
    }

    if (not router_list_.empty()) {
      flush_router_timer_.start(flush_interval_);
    }
  }

  void Ndp::flush_expired_neighbours()
  {
    PRINT("NDP: Flushing expired entries\n");
    std::vector<IP6::addr> expired;
    for (auto ent : neighbour_cache_) {
      if (ent.second.expired()) {
        expired.push_back(ent.first);
      }
    }

    for (auto ip : expired) {
      neighbour_cache_.erase(ip);
    }

    if (not neighbour_cache_.empty()) {
      flush_neighbour_timer_.start(flush_interval_);
    }
  }

  void Ndp::flush_expired_prefix()
  {
    PRINT("NDP: Flushing expired prefix addresses\n");
    for (auto ent = prefix_list_.begin(); ent != prefix_list_.end();) {
      if (!ent->valid()) {
        ent = prefix_list_.erase(ent);
      } else {
        ent++;
      }
    }

    if (not prefix_list_.empty()) {
      flush_prefix_timer_.start(flush_interval_);
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

  void Ndp::add_addr_static(ip6::Addr ip, uint32_t valid_lifetime)
  {
    auto entry = std::find_if(prefix_list_.begin(), prefix_list_.end(),
          [&ip] (const Prefix_entry& obj) { return obj.prefix() == ip; });

    if (entry == prefix_list_.end()) {
        prefix_list_.push_back(Prefix_entry{ip, 0, valid_lifetime});
    } else {
        entry->update_valid_lifetime(valid_lifetime);
    }
  }

  void Ndp::add_addr_onlink(ip6::Addr ip, uint32_t valid_lifetime)
  {
    auto entry = std::find_if(prefix_list_.begin(), prefix_list_.end(),
          [&ip] (const Prefix_entry& obj) { return obj.prefix() == ip; });

    if (entry == prefix_list_.end()) {
      if (valid_lifetime) {
        prefix_list_.push_back(Prefix_entry{ip, 0, valid_lifetime});
      }
    } else {
      if (valid_lifetime) {
        entry->update_valid_lifetime(valid_lifetime);
      } else {
        prefix_list_.erase(entry);
      }
    }
  }

  void Ndp::add_addr_autoconf(ip6::Addr ip, uint32_t preferred_lifetime, uint32_t valid_lifetime)
  {
    PRINT("NDP: RA: Adding address %s with preferred lifetime: %u"
            " and valid lifetime: %u\n", ip.to_string().c_str(),
            preferred_lifetime, valid_lifetime);

    auto entry = std::find_if(prefix_list_.begin(), prefix_list_.end(),
          [&ip] (const Prefix_entry& obj) { return obj.prefix() == ip; });
    auto two_hours = 60 * 60 * 2;

    if (entry == prefix_list_.end()) {
      prefix_list_.push_back(Prefix_entry{ip, preferred_lifetime, valid_lifetime});
    } else if (!entry->always_valid()) {
      entry->update_preferred_lifetime(preferred_lifetime);
      if ((valid_lifetime > two_hours) ||
          (valid_lifetime > entry->remaining_valid_time())) {
        /* Honor the valid lifetime only if its greater than 2 hours
         * or more than the remaining valid time */
        entry->update_valid_lifetime(valid_lifetime);
      } else if (entry->remaining_valid_time() > two_hours) {
        entry->update_valid_lifetime(two_hours);
      }
    }
  }

  void Ndp::add_router(ip6::Addr ip, uint16_t router_lifetime)
  {
    auto entry = std::find_if(router_list_.begin(), router_list_.end(),
          [&ip] (const Router_entry& obj) { return obj.router() == ip; });

    if (entry == router_list_.end()) {
      if (router_lifetime) {
        router_list_.push_back(Router_entry{ip, router_lifetime});
      }
    } else if (router_lifetime) {
      entry->update_router_lifetime(router_lifetime);
    } else {
      // Delete the destination cache entries which have
      // the next hop address equal to the router address
      delete_dest_entry(ip);
      router_list_.erase(entry);
    }
  }
} // net
