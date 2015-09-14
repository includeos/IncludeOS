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
  listeners.emplace(p, *this);
  listeners.at(p).listen(socket_backlog);
  debug("<TCP bind> Socket created and emplaced. State: %i\nThere are %i open ports. \n",
	listeners.at(p).poll(), listeners.size());
  return listeners.at(p);
}

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
