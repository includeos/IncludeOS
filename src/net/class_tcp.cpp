#define DEBUG
#define DEBUG2

#include <os>
#include <net/class_tcp.hpp>
#include <net/util.hpp>

using namespace net;

TCP::TCP(){
  debug2("<TCP::TCP> Instantiating \n");
  
}



int TCP::bottom(std::shared_ptr<Packet>& pckt){
 
  debug("<TCP::bottom> Upstream TCP-packet received \n");
  
  
  tcp_header* hdr = &((full_header*)pckt->buffer())->tcp_hdr;
  
  debug("<TCP::bottom> Destination port: %i, Source port: %i \n", 
	ntohs(hdr->dport), ntohs(hdr->sport));
  
  auto flags = ntohs(hdr->offs_flags.whole);
  auto raw_flags = hdr->offs_flags.whole;
  
  debug("<TCP::bottom> Flags raw: 0x%x, Flags reversed: 0x%x \n",raw_flags,flags);
  
  if (flags & (1 << SYN))
    debug("<TCP::bottom> SYN \n");
  
  if (flags & (1 << ACK))
    debug("<TCP::bottom> ACK \n");
  
  
  
  return 0;
  
}
