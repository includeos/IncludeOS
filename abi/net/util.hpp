#ifndef OS_NET_UTIL_HPP
#define OS_NET_UTIL_HPP

#include <stdint.h>

inline
uint16_t ntohs(uint16_t sh)
{
  return __builtin_bswap16(sh);
}
#define htons(x) ntohs(x)

#endif
