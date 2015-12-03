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

#define DEBUG // Allow debugging
#define DEBUG2 // Allow debugging

#include <os>
#include <net/arp.hpp>
#include <net/ip4/packet_arp.hpp>
#include <vector>
#include <net/inet4.hpp>

using namespace net;

int Arp::bottom(Packet_ptr pckt)
{
  debug2("<ARP handler> got %i bytes of data \n", pckt->size());

  header* hdr = (header*) pckt->buffer();
  //debug2("\t OPCODE: 0x%x \n",hdr->opcode);
  //debug2("Chaching IP %s for %s \n", hdr->sipaddr.str().c_str() , hdr->shwaddr.str().c_str())
  debug2("Have valid cache? %s \n",is_valid_cached(hdr->sipaddr) ? "YES":"NO");
  cache(hdr->sipaddr, hdr->shwaddr);
  
  switch(hdr->opcode){
    
  case H_request:
    debug2("\t ARP REQUEST: ");
    debug2("%s is looking for %s \n",
          hdr->sipaddr.str().c_str(),hdr->dipaddr.str().c_str());
    
    if (hdr->dipaddr == ip_)
      arp_respond(hdr);    
    else
    {
      debug2("\t NO MATCH for My IP (%s). DROP!\n",
          ip().str().c_str());
    }
    break;
    
  case H_reply:
    {
      debug2("\t ARP REPLY: %s belongs to %s\n",
	     hdr->sipaddr.str().c_str(), hdr->shwaddr.str().c_str());
      auto waiting = waiting_packets_.find(hdr->sipaddr);
      if (waiting != waiting_packets_.end()) {
	debug ("Had a packet waiting for this IP. Sending\n");
	transmit(waiting->second);
	waiting_packets_.erase(waiting);
      }
    }
    
    break;
    
  default:
    debug2("\t UNKNOWN OPCODE \n");
    break;
  }
  
  // Free the buffer (We're leaf node for this one's path)
  // @todo Freeing here corrupts the outgoing frame. Why?
  //free(data);
  
  return 0 + 0 * pckt->size(); // yep, it's what you think it is (and what's that?!)
};
  

void Arp::cache(IP4::addr& ip, Ethernet::addr& mac){
  
  debug2("Caching IP %s for %s \n",ip.str().c_str(),mac.str().c_str());
  
  auto entry = cache_.find(ip);
  if (entry != cache_.end()){
    
    debug2("Cached entry found: %s recorded @ %llu. Updating timestamp \n",
          entry->second.mac_.str().c_str(), entry->second.t_);
    
    // Update
    entry->second.update();
    
  }else cache_[ip] = mac; // Insert
  
}


bool Arp::is_valid_cached(IP4::addr& ip){
  auto entry = cache_.find(ip);
  
  
  if (entry != cache_.end()){
    debug("Cached entry, mac: %s time: %llu Expiry: %llu\n", 
	  entry->second.mac_.str().c_str(), entry->second.t_, entry->second.t_ + cache_exp_t_ );
    debug("Time now: %llu \n",(uint64_t)OS::uptime());
  }
  
  return entry != cache_.end() 
    and (entry->second.t_ + cache_exp_t_ > (uint64_t)OS::uptime());
}

extern "C" {
  unsigned long ether_crc(int length, unsigned char *data);
}



int Arp::arp_respond(header* hdr_in){
  debug2("\t IP Match. Constructing ARP Reply \n");
  
  // Populate ARP-header
  auto res = std::static_pointer_cast<PacketArp>(inet_.createPacket(sizeof(header)));
  res->init(mac_, ip_);
  
  res->set_dest_mac(hdr_in->shwaddr);
  res->set_dest_ip(hdr_in->sipaddr);
  res->set_opcode(H_reply);
    
  debug2("\t My IP: %s belongs to My Mac: %s \n",
	 res->source_ip().str().c_str(), res->source_mac().str().c_str());
  
  linklayer_out_(res);
  
  return 0;
}


static int ignore(std::shared_ptr<Packet> UNUSED(pckt)){
  debug2("<ARP -> linklayer> Empty handler - DROP!\n");
  return -1;
}


int Arp::transmit(Packet_ptr pckt){
  
  assert(pckt->size());
  
  /** Get destination IP from IP header   */
  IP4::ip_header* iphdr = (IP4::ip_header*)(pckt->buffer() 
                                            + sizeof(Ethernet::header));
  IP4::addr sip = iphdr->saddr;
  IP4::addr dip = pckt->next_hop();

  debug2("<ARP -> physical> Transmitting %i bytes to %s \n",
        pckt->size(),dip.str().c_str());
  
  Ethernet::addr mac;
  
  if (iphdr->daddr == IP4::INADDR_BCAST)
  {
    // when broadcasting our source IP should be either
    // our own IP or 0.0.0.0
    static const IP4::addr INADDR_NONE  {{0}};
    if (sip != ip_ && sip != INADDR_NONE)
    {
      debug2("<ARP> Dropping outbound broadcast packet due to "
             "invalid source IP %s\n",  sip.str().c_str());
      return -1;
    }
    mac = Ethernet::addr::BROADCAST_FRAME;
  }
  else
  {
    if (sip != ip_)
    {
      debug2("<ARP -> physical> Not bound to source IP %s. My IP is %s. DROP!\n",
            sip.str().c_str(), ip_.str().c_str());
      return -1;
    }
    
    // If we don't have a cached IP, get mac from next-hop (Håreks c001 hack)
    if (!is_valid_cached(dip))  
        return arp_resolver_(pckt);
    
    // Get mac from cache
    mac = cache_[dip].mac_;
  }
  
  /** Attach next-hop mac and ethertype to ethernet header  */  
  Ethernet::header* ethhdr = (Ethernet::header*)pckt->buffer();    
  ethhdr->src = mac_;
  ethhdr->dest.major = mac.major;
  ethhdr->dest.minor = mac.minor;
  ethhdr->type = Ethernet::ETH_IP4;
  
  debug2("<ARP -> physical> Sending packet to %s \n",mac.str().c_str());
  return linklayer_out_(pckt);
}

void Arp::await_resolution(Packet_ptr pckt, IP4::addr){
  auto queue =  waiting_packets_.find(pckt->next_hop());
  if (queue != waiting_packets_.end()) {
    debug("<ARP Resolve> Packets allready queueing for this IP\n");
    queue->second->chain(pckt);    
  } else {
    debug("<ARP Resolve> This is the first packet going to that IP\n");
    waiting_packets_.emplace(std::make_pair(pckt->next_hop(), pckt));
  }            
}

int Arp::arp_resolve(Packet_ptr pckt){
  debug("<ARP RESOLVE> %s \n", pckt->next_hop().str().c_str());
  
  await_resolution(pckt, pckt->next_hop());
  
  auto req = std::static_pointer_cast<PacketArp>(inet_.createPacket(sizeof(header)));
  req->init(mac_, ip_);
  
  req->set_dest_mac(Ethernet::addr::BROADCAST_FRAME);
  req->set_dest_ip(pckt->next_hop());
  req->set_opcode(H_request);
  
  linklayer_out_(req);
  
  return 0;
}

// Initialize
Arp::Arp(net::Inet<Ethernet,IP4>& inet): 
  inet_(inet), mac_(inet.link_addr()), ip_(inet.ip_addr()), 
  linklayer_out_(downstream(ignore))
{}

int Arp::hh_map(Packet_ptr pckt){
  (void) pckt;
  debug("ARP-resolution using the HH-hack");
  return 0;
    /**
     // Fixed mac prefix
     mac.minor = 0x01c0; //Big-endian c001
     // Destination IP
     mac.major = dip.whole;
     debug("ARP cache missing. Guessing Mac %s from next-hop IP: %s (dest.ip: %s)",
     mac.str().c_str(), dip.str().c_str(), iphdr->daddr.str().c_str());
    **/
}
