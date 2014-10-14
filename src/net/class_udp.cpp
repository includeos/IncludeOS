#include <os>
#include <net/class_udp.hpp>

int UDP::bottom(uint8_t* data, int len){
  printf("<UDP handler> Got data \n");
  
  header* hdr = (header*)data;
  printf("\t Source port: %i, Dest. Port: %i Length: %i\n",
         __builtin_bswap16(hdr->sport),__builtin_bswap16(hdr->dport), 
         __builtin_bswap16(hdr->length));
  
  printf("\t Content: '%s' \n",(char*)data+sizeof(header));
  
  
  return -1;
}
