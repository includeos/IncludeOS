#define DEBUG
#define DEBUG2

#include <os>
#include <net/class_tcp.hpp>
#include <net/util.hpp>

using namespace net;

TCP::TCP()
  : listeners() 
{
  debug2("<TCP::TCP> Instantiating. Open ports: %i \n", listeners.size()); 
}


TCP::Socket& TCP::bind(port p){
  auto listener = listeners.find(p);
  
  if (listener != listeners.end())
    panic("Port busy!");
  
  debug("<TCP bind> listening to port %i \n",p);
  // Create a socket and allow it to know about this stack
  
  listeners.emplace(p,TCP::Socket(*this, p, TCP::Socket::CLOSED));

  listeners.at(p).listen(socket_backlog);
  debug("<TCP bind> Socket created and emplaced. State: %i\nThere are %i open ports. \n",
	listeners.at(p).poll(), listeners.size());
  return listeners.at(p);
}


uint16_t TCP::checksum(full_header* hdr){  
  // Size has to be fetched from the frame
  debug2("<TCP::checksum> Checksumming header of size %i \n",sizeof(tcp_header));
  
  IP4::ip_header ip_hdr = hdr->ip_hdr;
  
  pseudo_header pseudo_hdr;

  pseudo_hdr.saddr.whole = ip_hdr.saddr.whole;
  pseudo_hdr.daddr.whole = ip_hdr.daddr.whole;
  pseudo_hdr.zero = 0;
  pseudo_hdr.proto = IP4::IP4_TCP;
  // TODO: Figure out how the amount of data is calculated.
  pseudo_hdr.tcp_length = sizeof(tcp_header);
    
  return net::checksum((uint16_t*)hdr,sizeof(tcp_header));
  
}


void TCP::set_offset(tcp_header* hdr, uint8_t offset){  
  offset <<= 4;
  hdr->offs_flags.offs_res = offset;
}

int TCP::transmit(std::shared_ptr<Packet>& pckt){
  
  tcp_header* hdr = &((full_header*)pckt->buffer())->tcp_hdr;
  set_offset(hdr, 5);
  hdr->checksum = 0;
  //hdr->checksum = checksum(hdr);
  return _network_layer_out(pckt);
};


int TCP::bottom(std::shared_ptr<Packet>& pckt){
 
  debug("<TCP::bottom> Upstream TCP-packet received, to TCP @ %p \n", this);
  debug("<TCP::bottom> There are %i open ports \n", listeners.size());
  
  tcp_header* hdr = &((full_header*)pckt->buffer())->tcp_hdr;
  
  debug("<TCP::bottom> Destination port: %i, Source port: %i \n", 
	ntohs(hdr->dport), ntohs(hdr->sport));
  
  auto listener = listeners.find(ntohs(hdr->dport));
  
  if (listener == listeners.end()){
    debug("<TCP::bottom> Nobody's listening to this port. Ignoring");
    debug("<TCP::bottom> There are %i open ports \n", listeners.size());
    return 0;
  }
  
  debug("<TCP::bottom> Somebody's listening to this port. Passing it up to the socket");
  
  (*listener).second.bottom(pckt);
  
  
  return 0;
  
}
