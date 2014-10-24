#include <os>
#include <net/inet.hpp>


uint16_t net::checksum(uint16_t* buf, uint32_t len){
  union sum{
    uint32_t whole;    
    uint16_t part[2];
  }sum32{0};

  
  // Iterate in short int steps.
  for (uint16_t* i = buf; i < buf + len / 2; i++){
    sum32.whole += *i;
  }
  
  // We're not checking for the odd-length case yet
  
  return ~(sum32.part[0]+sum32.part[1]);
};
