#define DEBUG
#define DEBUG2

#include <os>
#include <net/class_tcp.hpp>
#include <net/util.hpp>

using namespace net;

TCP::TCP(IP4::addr ip)
  : local_ip_(ip), listeners()
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


uint16_t TCP::checksum(std::shared_ptr<Packet>& pckt){  
  // Size has to be fetched from the frame
  
  full_header* hdr = (full_header*)pckt->buffer();
  
  IP4::ip_header* ip_hdr = &hdr->ip_hdr;
  tcp_header* tcp_hdr = &hdr->tcp_hdr;
  pseudo_header pseudo_hdr;
  
  int tcp_header_size = header_len(tcp_hdr);
  int all_headers_size = (sizeof(full_header) - sizeof(tcp_header)) + tcp_header_size;
  int data_size = pckt->len() - all_headers_size;
  int tcp_length = tcp_header_size + data_size;
  
  pseudo_hdr.saddr.whole = ip_hdr->saddr.whole;
  pseudo_hdr.daddr.whole = ip_hdr->daddr.whole;
  pseudo_hdr.zero = 0;
  pseudo_hdr.proto = IP4::IP4_TCP;

  debug2("<TCP::checksum> Packet size: %i, TCP_hdr_len: %i, sizeof(full_header): %i, sizeof(tcp_header): %i, all_hdrs_len: %i, data_len: %i, tcp_seg_size %i \n",
	 pckt->len(),tcp_header_size, sizeof(full_header), sizeof(tcp_header),all_headers_size, data_size, tcp_length);
  debug("Proto TCP::IP4_TCP = %p, Proto on wire = %p \n",IP4::IP4_TCP, ip_hdr->protocol);

  
  pseudo_hdr.tcp_length = htons(tcp_length);
  
  
  union {
    uint32_t whole;
    uint16_t part[2];
  }sum;
  
  sum.whole = 0;
  
  // Compute sum for pseudo header
  for (uint16_t* it = (uint16_t*)&pseudo_hdr; it < (uint16_t*)&pseudo_hdr + sizeof(pseudo_hdr)/2; it++)
    sum.whole += *it;
  
  // Compute sum for the actual header (and data, but none for now)
  for (uint16_t* it = (uint16_t*)tcp_hdr; it < (uint16_t*)tcp_hdr + tcp_length/2; it++)
    sum.whole+= *it;
      
  debug2("<TCP::checksum: sum: %p, half+halp: %p, TCP checksum: %p, TCP checksum big-endian: %p \n",
	 sum.whole, sum.part[0] + sum.part[1], (uint16_t)~((uint16_t)(sum.part[0] + sum.part[1])), htons((uint16_t)~((uint16_t)(sum.part[0] + sum.part[1]))));
  return ~(sum.part[0] + sum.part[1]);
  
}


uint8_t TCP::get_offset(tcp_header* hdr){
  return (uint8_t)(hdr->offs_flags.offs_res >> 4);
}

uint8_t TCP::header_len(tcp_header* hdr){
  return get_offset(hdr) * 4;
}


void TCP::set_offset(tcp_header* hdr, uint8_t offset){  
  offset <<= 4;
  hdr->offs_flags.offs_res = offset;
}

int TCP::transmit(std::shared_ptr<Packet>& pckt){
  
  full_header* full_hdr = (full_header*)pckt->buffer();
  tcp_header* hdr = &(full_hdr->tcp_hdr);
  
  // Set source address
  full_hdr->ip_hdr.saddr.whole = local_ip_.whole;
  
  set_offset(hdr, 5);
  hdr->checksum = 0;  
  hdr->checksum = checksum(pckt);
  return _network_layer_out(pckt);
};


int TCP::bottom(std::shared_ptr<Packet>& pckt){
 
  debug("<TCP::bottom> Upstream TCP-packet received, to TCP @ %p \n", this);
  debug("<TCP::bottom> There are %i open ports \n", listeners.size());
  
  full_header* full_hdr = (full_header*)pckt->buffer();
  tcp_header* hdr = &full_hdr->tcp_hdr;
  
  debug("<TCP::bottom> Incoming packet TCP-checksum: %p \n", ntohs(checksum(pckt)));
  
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
