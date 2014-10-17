//#define NDEBUG // Supress debugging
#include <os>
#include <net/class_udp.hpp>

int UDP::bottom(uint8_t* data, int len){
  debug("<UDP handler> Got data \n");
  
  header* hdr = (header*)data;
  debug("\t Source port: %i, Dest. Port: %i Length: %i\n",
         __builtin_bswap16(hdr->sport),__builtin_bswap16(hdr->dport), 
         __builtin_bswap16(hdr->length));

  auto l = ports.find(__builtin_bswap16(hdr->dport));
  if (l != ports.end()){
    debug("<UDP> Someone's listening to this port. Let them hear it.\n");
    return l->second(data,len);
  }
  
  debug("<UDP> Nobody's listening to this port. Drop!\n");
    
  
  return -1;
}

void UDP::listen(uint16_t port, listener l){
  debug("<UDP> Listening to port %i \n",port);
  
  // any previous listeners will be evicted.
  ports[port] = l;
  
};

int UDP::transmit(IP4::addr sip,UDP::port sport,
                  IP4::addr dip,UDP::port dport,
                  uint8_t* data, int len){
  
  debug("<UDP> Transmitting %i bytes to %s:%i \n",len,dip.str().c_str(),dport);
  assert((uint32_t)len >= sizeof(UDP::header));
  
  UDP::header* hdr = (UDP::header*)data;
  hdr->dport = dport;
  hdr->sport = sport;

  return _network_layer_out(sip,dip,IP4::IP4_UDP,data,len);

};

int ignore_udp(IP4::addr UNUSED(sip),IP4::addr UNUSED(dip),IP4::proto UNUSED(p),
               UDP::pbuf& UNUSED(buf),uint32_t UNUSED(len)){
  debug("<UDP->Network> No handler - DROP!\n");
  return 0;
}

UDP::UDP() : _network_layer_out(UDP::network_out(ignore_udp)) {};
