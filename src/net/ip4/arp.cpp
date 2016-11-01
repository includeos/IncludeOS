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

#define DEBUG  // Allow debugging
#define DEBUG2 // Allow debugging

#include <vector>

#include <os>
#include <net/inet4.hpp>
#include <net/ip4/arp.hpp>
#include <net/ip4/packet_arp.hpp>
#include <statman>

namespace net {

  static void ignore(Packet_ptr) {
    debug2("<ARP -> linklayer> Empty handler - DROP!\n");
  }

  // Initialize
  Arp::Arp(Stack& inet) noexcept:
  requests_rx_    {Statman::get().create(Stat::UINT32, inet.ifname() + ".arp.requests_rx").get_uint32()},
  requests_tx_    {Statman::get().create(Stat::UINT32, inet.ifname() + ".arp.requests_tx").get_uint32()},
  replies_rx_     {Statman::get().create(Stat::UINT32, inet.ifname() + ".arp.replies_rx").get_uint32()},
  replies_tx_     {Statman::get().create(Stat::UINT32, inet.ifname() + ".arp.replies_tx").get_uint32()},
  inet_           {inet},
  mac_            (inet.link_addr()),
  linklayer_out_  {ignore}
  {}

  void Arp::bottom(Packet_ptr pckt) {
    debug2("<ARP handler> got %i bytes of data\n", pckt->size());

    header* hdr = reinterpret_cast<header*>(pckt->buffer());

    debug2("Have valid cache? %s\n", is_valid_cached(hdr->sipaddr) ? "YES" : "NO");
    cache(hdr->sipaddr, hdr->shwaddr);

    switch(hdr->opcode) {

    case H_request: {
      // Stat increment requests received
      requests_rx_++;

      debug2("\t ARP REQUEST: ");
      debug2("%s is looking for %s\n",
             hdr->sipaddr.str().c_str(),
             hdr->dipaddr.str().c_str());

      if (hdr->dipaddr == inet_.ip_addr()) {
        arp_respond(hdr);
      } else {
        debug2("\t NO MATCH for My IP (%s). DROP!\n",
               inet_.ip_addr().str().c_str());
      }
      break;
    }

    case H_reply: {
      // Stat increment replies received
      replies_rx_++;

      debug2("\t ARP REPLY: %s belongs to %s (waiting: %u)\n",
             hdr->sipaddr.str().c_str(), hdr->shwaddr.str().c_str(), waiting_packets_.size());

      auto waiting = waiting_packets_.find(hdr->sipaddr);

      if (waiting != waiting_packets_.end()) {
        debug("Had a packet waiting for this IP. Sending\n");
        transmit(std::move(waiting->second));
        waiting_packets_.erase(waiting);
      }
      break;
    }

    default:
      debug2("\t UNKNOWN OPCODE\n");
      break;
    } //< switch(hdr->opcode)
  }

  void Arp::cache(IP4::addr ip, Ethernet::addr mac) {
    debug2("Caching IP %s for %s\n", ip.str().c_str(), mac.str().c_str());

    auto entry = cache_.find(ip);

    if (entry != cache_.end()) {
      debug2("Cached entry found: %s recorded @ %llu. Updating timestamp\n",
             entry->second.mac_.str().c_str(), entry->second.timestamp_);

      // Update
      entry->second.update();

    } else {
      cache_[ip] = mac; // Insert
    }
  }

  bool Arp::is_valid_cached(IP4::addr ip) {
    auto entry = cache_.find(ip);

    if (entry != cache_.end()) {
      debug("Cached entry, mac: %s time: %llu Expiry: %llu\n",
            entry->second.mac_.str().c_str(),
            entry->second.timestamp_, entry->second.timestamp_ + cache_exp_t_);
      debug("Time now: %llu\n", static_cast<uint64_t>(OS::uptime()));
    }

    return entry != cache_.end()
      and (entry->second.timestamp_ + cache_exp_t_ > static_cast<uint64_t>(OS::uptime()));
  }

  extern "C" {
    unsigned long ether_crc(int length, unsigned char *data);
  }

  void Arp::arp_respond(header* hdr_in) {
    debug2("\t IP Match. Constructing ARP Reply\n");

    // Stat increment replies sent
    replies_tx_++;

    // Populate ARP-header
    auto res = static_unique_ptr_cast<PacketArp>(inet_.create_packet(sizeof(header)));
    res->init(mac_, inet_.ip_addr());

    res->set_dest_mac(hdr_in->shwaddr);
    res->set_dest_ip(hdr_in->sipaddr);
    res->set_opcode(H_reply);

    debug2("\t My IP: %s belongs to My Mac: %s\n",
           res->source_ip().str().c_str(), res->source_mac().str().c_str());

    linklayer_out_(std::move(res));
  }

  void Arp::transmit(Packet_ptr pckt) {
    assert(pckt->size());

    /** Get destination IP from IP header */
    IP4::ip_header* iphdr = reinterpret_cast<IP4::ip_header*>(pckt->buffer()
                                                              + sizeof(Ethernet::header));
    IP4::addr sip = iphdr->saddr;
    IP4::addr dip = pckt->next_hop();

    debug2("<ARP -> physical> Transmitting %i bytes to %s\n",
           pckt->size(), dip.str().c_str());

    Ethernet::addr dest_mac;

    if (iphdr->daddr == IP4::ADDR_BCAST) {
      // When broadcasting our source IP should be either
      // our own IP or 0.0.0.0

      if (sip != inet_.ip_addr() && sip != IP4::ADDR_ANY) {
        debug2("<ARP> Dropping outbound broadcast packet due to "
               "invalid source IP %s\n",  sip.str().c_str());
        return;
      }
      // mui importante
      dest_mac = Ethernet::BROADCAST_FRAME;

    } else {
      if (sip != inet_.ip_addr()) {
        debug2("<ARP -> physical> Not bound to source IP %s. My IP is %s. DROP!\n",
               sip.str().c_str(), inet_.ip_addr().str().c_str());
        return;
      }

      // If we don't have a cached IP, perform address resolution
      if (!is_valid_cached(dip)) {
        arp_resolver_(std::move(pckt));
        return;
      }

      // Get MAC from cache
      dest_mac = cache_[dip].mac_;
    }

    /** Attach next-hop mac and ethertype to ethernet header */
    auto* ethhdr = reinterpret_cast<Ethernet::header*>(pckt->buffer());
    ethhdr->src  = mac_;
    ethhdr->dest = dest_mac;
    ethhdr->type = Ethernet::ETH_IP4;

    /** Update chain as well */
    auto* next = pckt->tail();
    while(next) {
      auto* headur = reinterpret_cast<Ethernet::header*>(next->buffer());
      headur->src  = mac_;
      headur->dest = dest_mac;
      headur->type = Ethernet::ETH_IP4;
      next = next->tail();
    }

    debug2("<ARP -> physical> Sending packet to %s\n", mac_.str().c_str());
    linklayer_out_(std::move(pckt));
  }

  void Arp::await_resolution(Packet_ptr pckt, IP4::addr) {
    auto queue =  waiting_packets_.find(pckt->next_hop());

    if (queue != waiting_packets_.end()) {
      debug("<ARP Resolve> Packets already queueing for this IP\n");
      queue->second->chain(std::move(pckt));
    } else {
      debug("<ARP Resolve> This is the first packet going to that IP\n");
      waiting_packets_.emplace(std::make_pair(pckt->next_hop(), std::move(pckt)));
    }
  }

  void Arp::arp_resolve(Packet_ptr pckt) {
    debug("<ARP RESOLVE> %s\n", pckt->next_hop().str().c_str());
    const auto next_hop = pckt->next_hop();
    await_resolution(std::move(pckt), next_hop);

    auto req = static_unique_ptr_cast<PacketArp>(inet_.create_packet(sizeof(header)));
    req->init(mac_, inet_.ip_addr());

    req->set_dest_mac(Ethernet::BROADCAST_FRAME);
    req->set_dest_ip(next_hop);
    req->set_opcode(H_request);

    // Stat increment requests sent
    requests_tx_++;

    linklayer_out_(std::move(req));
  }

  void Arp::hh_map(Packet_ptr pckt) {
    (void) pckt;
    debug("ARP-resolution using the HH-hack");
    /**
     // Fixed mac prefix
     mac.minor = 0x01c0; //Big-endian c001
     // Destination IP
     mac.major = dip.whole;
     debug("ARP cache missing. Guessing Mac %s from next-hop IP: %s (dest.ip: %s)",
     mac.str().c_str(), dip.str().c_str(), iphdr->daddr.str().c_str());
    **/
  }

} //< namespace net
