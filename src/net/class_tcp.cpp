#define DEBUG
#define DEBUG2

#include <os>
#include <net/class_tcp.hpp>
#include <net/util.hpp>

using namespace net;

TCP::Socket::Socket(TCP& tcp) : local_stack_(tcp)
{}

void TCP::Socket::listen(int backlog){
  state_ = LISTEN;
  backlog_ = backlog;
  
  // Possibly allocate connectoin pointers (but for now we're using map)
  
};

int TCP::Socket::bottom(std::shared_ptr<Packet>& pckt){
  tcp_header* hdr = &((full_header*)pckt->buffer())->tcp_hdr;
  auto flags = ntohs(hdr->offs_flags.whole);
  auto raw_flags = hdr->offs_flags.whole;
  
  debug("<TCP::Socket::bottom> Flags raw: 0x%x, Flags reversed: 0x%x \n",raw_flags,flags);
  
  if (flags & (1 << SYN)) {       
    
    if (! (flags & (1 << ACK))) {
      debug("<TCP::Socket::bottom> SYN-packet; new connection \n");
      assert(state_ == LISTEN);
      
      if (connections.size() >= backlog_) {
	debug("<TCP::Socket::bottom> DROP. We don't have room for more connections.");
	return 0;
      }
      
    }else {
      debug("<TCP::Socket::bottom> SYN-ACK; \n");
      assert(state_ == SYN_RECIEVED);		
    }    
  }     
  
  if (flags & (1 << ACK))
    debug("<TCP::Socket::bottom> ACK \n");

;  
  
  return 0;
}

TCP::TCP(){
  debug2("<TCP::TCP> Instantiating \n");
  
}


TCP::Socket& TCP::bind(port p){
  auto listener = listeners.find(p);
  
  if (listener != listeners.end())
    panic("Port busy!");
  
  debug("<TCP bind> listening to port %i \n",p);
  // Create a socket and allow it to know about this stack
  listeners.emplace(p, TCP::Socket (*this));
  listeners.at(p).listen(socket_backlog);
  debug("<TCP bind> Socket created and emplaced. State: %i \n",
	listeners.at(p).poll());
  return listeners.at(p);
}

int TCP::bottom(std::shared_ptr<Packet>& pckt){
 
  debug("<TCP::bottom> Upstream TCP-packet received \n");
  
  
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
