//#define DEBUG
#include <os>
#include <net/inet_common.hpp>
#include <net/icmp.hpp>

using namespace net;

int ICMP::bottom(std::shared_ptr<Packet>& pckt){

  if (pckt->len() < sizeof(full_header)) //Drop if not a full header.
    return -1;
  
  full_header* full_hdr = (full_header*)pckt->buffer();
  icmp_header* hdr = &full_hdr->icmp_hdr;
  
  switch(hdr->type)
  {
  case (ICMP_ECHO):
    debug("<ICMP> PING from %s \n",full_hdr->ip_hdr.saddr.str().c_str());
    ping_reply(full_hdr);
    break;
  case (ICMP_ECHO_REPLY):
    debug("<ICMP> PING Reply from %s \n",full_hdr->ip_hdr.saddr.str().c_str());
    break;
  }
  
  return 0;
}


void ICMP::ping_reply(full_header* full_hdr){

  
  /** @todo we're now reusing the same buffer every time. 
      If the last reply isn't sent by now, it will get overwritten*/
  memset(buf,0,sizeof(full_header));
  
  icmp_header* hdr = &((full_header*)buf)->icmp_hdr;
  hdr->type = ICMP_ECHO_REPLY;  
  hdr->rest = full_hdr->icmp_hdr.rest;
  hdr->checksum = 0;
  hdr->checksum = net::checksum((uint16_t*)hdr,sizeof(icmp_header));

  debug("<ICMP> Rest of header IN: 0x%lx OUT: 0x%lx \n",
        full_hdr->icmp_hdr.rest, hdr->rest);
  
  debug("<ICMP> Transmitting answer\n");

  /** Populate response IP header */
  IP4::ip_header* dst_ip_hdr = (IP4::ip_header*)(buf + sizeof(Ethernet::header));
  dst_ip_hdr->saddr = full_hdr->ip_hdr.daddr;
  dst_ip_hdr->daddr = full_hdr->ip_hdr.saddr;
  dst_ip_hdr->protocol = IP4::IP4_ICMP;
  
  /** Create packet */
  auto packet_ptr = std::make_shared<Packet>
    (Packet(buf, sizeof(full_header), Packet::DOWNSTREAM));

  _network_layer_out(packet_ptr);
}

int icmp_ignore(std::shared_ptr<Packet> UNUSED(pckt)){
  debug("<ICMP IGNORE> No handler. DROP!\n");
  return -1;
}

ICMP::ICMP() : 
  _network_layer_out(downstream(icmp_ignore))
{
  buf = (uint8_t*)malloc(sizeof(full_header));
}
