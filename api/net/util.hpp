#pragma once
#include <stdint.h>

#ifdef DEBUG
#define debug(X,...)  printf(X,##__VA_ARGS__);
#else 
#define debug(X,...) 
#endif

namespace net
{
  /*
   * See P.49 of C programming
   * Get "n" bits from integer "x", starting from position "p"
   * e.g., getbits(x, 31, 8) -- highest byte
   *       getbits(x,  7, 8) -- lowest  byte
   */
  #define getbits(x, p, n) ((x >> (p+1-n)) & ~(~0 << n))
  
  inline
  uint16_t ntohs(uint16_t n)
  {
    return __builtin_bswap16(n);
  }
  inline
  uint16_t htons(uint16_t n)
  {
    return __builtin_bswap16(n);
  }
  
  inline
  uint32_t ntohl(uint32_t n)
  {
    return __builtin_bswap32(n);
  }
  inline
  uint32_t htonl(uint32_t n)
  {
    return __builtin_bswap32(n);
  }
  
  inline
  uint64_t ntohll(uint64_t n)
  {
    return __builtin_bswap64(n);
  }
  inline
  uint64_t htonll(uint64_t n)
  {
    return __builtin_bswap64(n);
  }
  
}
