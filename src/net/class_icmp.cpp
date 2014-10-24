#define DEBUG
#include <os>
#include <net/class_icmp.hpp>

using namespace net;

int ICMP::bottom(uint8_t* data, int UNUSED(len))
{
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
  // @todo We can't just assume the last one has been sent by now... but we do.
  if (buf)
    free(buf);
  
  buf = (uint8_t*)malloc(sizeof(full_header));
  memset(buf,0,sizeof(full_header));
  
  icmp_header* hdr = (icmp_header*)buf;
  hdr->type = ICMP_ECHO_REPLY;
  
  hdr->checksum = 0xffff;
  
  debug("<ICMP> Transmitting answer\n");
  _network_layer_out(full_hdr->ip_hdr.daddr,full_hdr->ip_hdr.saddr,
                     IP4::IP4_ICMP,buf,sizeof(full_header));
}

int icmp_ignore(IP4::addr UNUSED(source_ip), IP4::addr UNUSED(dest_ip), 
                IP4::proto UNUSED(proto),    IP4::pbuf UNUSED(buffer), 
                uint32_t UNUSED(len))
{
	debug("<ICMP IGNORE> No handler. DROP!\n");
	return 0;
}

ICMP::ICMP() : 
  _network_layer_out(IP4::transmitter(icmp_ignore)){}
