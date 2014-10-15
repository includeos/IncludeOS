#define NDEBUG // Supress debugging
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
  printf("<UDP> Listening to port %i \n",port);
  
  // any previous listeners will be evicted.
  ports[port] = l;

};

