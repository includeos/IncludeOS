#include <os>
#include <net/inet_common.hpp>
#include <net/util.hpp>
#include <stdlib.h>

// Should be pretty much like the example in RFC 1071, but using a uinon for readability
uint16_t net::checksum(void* data, size_t len)
{
  uint16_t* buf = (uint16_t*)data;

  union sum
  {
    uint32_t whole;    
    uint16_t part[2];
  } sum32{0};
  
  // Iterate in short int steps.
  for (uint16_t* i = buf; i < buf + len / 2; i++)
    sum32.whole += *i;
  
  // odd-length case
  if (len & 1)    
    sum32.whole += ((uint8_t*) buf)[len-1];

  return ~(sum32.part[0]+sum32.part[1]);
}
