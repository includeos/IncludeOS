//#define DEBUG // Allow debugging
#include <os>
#include <net/class_udp.hpp>

using namespace net;

int UDP::bottom(std::shared_ptr<Packet>& pckt){
  debug("<UDP handler> Got data \n");
  
  
  udp_header* hdr = &((full_header*)pckt->buffer())->udp_hdr;
  
  debug("\t Source port: %i, Dest. Port: %i Length: %i\n",
        __builtin_bswap16(hdr->sport),__builtin_bswap16(hdr->dport), 
        __builtin_bswap16(hdr->length));
  
  auto l = ports.find(__builtin_bswap16(hdr->dport));
  if (l != ports.end()){
    debug("<UDP> Someone's listening to this port. Let them hear it.\n");
    return l->second(pckt);
  }
  
  debug("<UDP> Nobody's listening to this port. Drop!\n");
  
  
  return -1;
}

void UDP::listen(uint16_t port, listener l){
  debug("<UDP> Listening to port %i \n",port);
  
  // any previous listeners will be evicted.
  ports[port] = l;
  
};

/*int UDP::transmit(IP4::addr sip,UDP::port sport,
  IP4::addr dip,UDP::port dport,
  uint8_t* data, int len){*/

int UDP::transmit(std::shared_ptr<Packet>& pckt){
  
  
  ASSERT((uint32_t)pckt->len() >= sizeof(UDP::full_header));
  
  full_header* full_hdr = (full_header*)pckt->buffer();
  udp_header* hdr = &(full_hdr->udp_hdr);
  
  // Populate all UDP header fields
  /**
     hdr->dport = dport;
     hdr->sport = sport; */
  
  hdr->length =  __builtin_bswap16((uint16_t)(pckt->len() 
                                              - sizeof(IP4::full_header)));
  hdr->checksum = 0; // This field is optional (must be 0 if not used)

  IP4::addr sip = full_hdr->ip_hdr.saddr;
  IP4::addr dip = full_hdr->ip_hdr.daddr;
  
  debug("<UDP> Transmitting %i bytes (big-endian 0x%x) to %s:%i \n",
        (uint16_t)(pckt->len() -sizeof(full_header)),
        hdr->length,dip.str().c_str(),
        dport);
  
  ASSERT(sip != 0 && dip != 0 &&
         full_hdr->ip_hdr.protocol == IP4::IP4_UDP);
  
  debug("<UDP> sip: %s dip: %s, type: %i, len: %i  \n ",
        sip.str().c_str(),dip.str().c_str(),IP4::IP4_UDP,len
        );
  
  
  return _network_layer_out(pckt);
  
};

int ignore_udp(std::shared_ptr<Packet> UNUSED(pckt)){
  debug("<UDP->Network> No handler - DROP!\n");
  return 0;
}

UDP::UDP() : _network_layer_out(downstream(ignore_udp)) {};
