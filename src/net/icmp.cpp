// Part of the IncludeOS Unikernel  - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and  Alfred Bratterud. 
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


//#define DEBUG
#include <os>
#include <net/ip4/packet_ip4.hpp>
#include <net/inet_common.hpp>
#include <net/icmp.hpp>
#include <net/util.hpp>

using namespace net;

int ICMP::bottom(Packet_ptr pckt){

  if (pckt->size() < sizeof(full_header)) //Drop if not a full header.
    return -1;
  
  full_header* full_hdr = (full_header*) pckt->buffer();
  icmp_header* hdr = &full_hdr->icmp_hdr;
  
  switch(hdr->type)
  {
  case (ICMP_ECHO):
    debug("<ICMP> PING from %s \n",full_hdr->ip_hdr.saddr.str().c_str());
    ping_reply(full_hdr, pckt->size());
    break;
  case (ICMP_ECHO_REPLY):
    debug("<ICMP> PING Reply from %s \n",full_hdr->ip_hdr.saddr.str().c_str());
    break;
  }
  
  return 0;
}


void ICMP::ping_reply(full_header* full_hdr, uint16_t size)
{
  auto packet_ptr = inet_.createPacket(size);
  auto buf = packet_ptr->buffer();
  
  icmp_header* hdr = &((full_header*)buf)->icmp_hdr;
  hdr->type = ICMP_ECHO_REPLY;
  hdr->code = 0;
  hdr->identifier = full_hdr->icmp_hdr.identifier;
  hdr->sequence   = full_hdr->icmp_hdr.sequence;

  debug("<ICMP> Rest of header IN: 0x%lx OUT: 0x%lx \n",
        full_hdr->icmp_hdr.rest, hdr->rest);
  
  debug("<ICMP> Transmitting answer\n");

  /** Populate response IP header */
  auto ip4_pckt = std::static_pointer_cast<PacketIP4>(packet_ptr);
  ip4_pckt->init();
  ip4_pckt->set_src(full_hdr->ip_hdr.daddr);
  ip4_pckt->set_dst(full_hdr->ip_hdr.saddr);
  ip4_pckt->set_protocol(IP4::IP4_ICMP);
  
  // copy payload from old to new packet
  uint8_t* payload = ((uint8_t*) hdr) + sizeof(icmp_header);
  uint8_t* source  = (((uint8_t*) &full_hdr->icmp_hdr) + sizeof(icmp_header));
  memcpy(payload, source, size - sizeof(full_header));
  
  hdr->checksum = 0;
  hdr->checksum = net::checksum((uint16_t*)hdr, 
      size - sizeof(full_header) + sizeof(icmp_header));
  
  _network_layer_out(packet_ptr);
}

int net::icmp_default_out(std::shared_ptr<Packet> UNUSED(pckt)){
  debug("<ICMP IGNORE> No handler. DROP!\n");
  return -1;
}

ICMP::ICMP(Inet<LinkLayer,IP4>& inet) 
  : inet_(inet) {}
