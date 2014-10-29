//#define DEBUG
#include <os>
#include <net/inet.hpp>
#include <net/class_icmp.hpp>

using namespace net;

int ICMP::bottom(uint8_t* data, int len){

  if ((uint32_t)len < sizeof(full_header)) //Drop if not a full header.
    return -1;
  
  full_header* full_hdr = (full_header*)data;
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
  _network_layer_out(full_hdr->ip_hdr.daddr,full_hdr->ip_hdr.saddr,
                     IP4::IP4_ICMP,buf,sizeof(full_header));
}

int icmp_ignore(IP4::addr UNUSED(sip),IP4::addr UNUSED(dip), 
                IP4::proto UNUSED(p), IP4::pbuf UNUSED(buf), 
                uint32_t UNUSED(len)){
  debug("<ICMP IGNORE> No handler. DROP!\n");
  return -1;
}

ICMP::ICMP() : 
  _network_layer_out(IP4::transmitter(icmp_ignore))
{
buf = (uint8_t*)malloc(sizeof(full_header));
}
