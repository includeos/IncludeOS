#include <os>
#include <net/inet.hpp>
#include <net/util.hpp>
#include <stdlib.h>

/*
uint16_t net::checksum(uint16_t* buf, uint32_t len)
{
  union sum
  {
    uint32_t whole;    
    uint16_t part[2];
  } sum32{0};
  
  // Iterate in short int steps.
  for (uint16_t* i = buf; i < buf + len / 2; i++){
    sum32.whole += *i;
  }
  
  // odd-length case
  if (len & 1)
  {
    //printf("odd case\n");
    sum32.whole += ((uint8_t*) buf)[len-1];
  }
  
  return ~(sum32.part[0]+sum32.part[1]);
}
*/

/**
 * http://www.microhowto.info/howto/calculate_an_internet_protocol_checksum_in_c.html
 * by Graham Shaw, some rights reserved. 
**/

uint16_t net::checksum(void* vdata, size_t length)
{
    // Cast the data pointer to one that can be indexed.
    char* data=(char*)vdata;
    
    // Initialise the accumulator.
    uint64_t acc=0xffff;
    
    // Handle any partial block at the start of the data.
    unsigned int offset=((uintptr_t)data)&3;
    if (offset) {
        size_t count=4-offset;
        if (count>length) count=length;
        uint32_t word=0;
        memcpy(offset+(char*)&word,data,count);
        acc+=ntohl(word);
        data+=count;
        length-=count;
    }
    
    // Handle any complete 32-bit blocks.
    char* data_end=data+(length&~3);
    while (data!=data_end) {
        uint32_t word;
        memcpy(&word,data,4);
        acc+=ntohl(word);
        data+=4;
    }
    length&=3;
    
    // Handle any partial block at the end of the data.
    if (length) {
        uint32_t word=0;
        memcpy(&word,data,length);
        acc+=ntohl(word);
    }
    
    // Handle deferred carries.
    acc=(acc&0xffffffff)+(acc>>32);
    while (acc>>16) {
        acc=(acc&0xffff)+(acc>>16);
    }
    
    // If the data began at an odd byte address
    // then reverse the byte order to compensate.
    if (offset&1) {
        acc=((acc&0xff00)>>8)|((acc&0x00ff)<<8);
    }
    
    // Return the checksum in network byte order.
    return htons(~acc);
}
