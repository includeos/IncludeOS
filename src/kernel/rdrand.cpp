#include <kernel/rdrand.hpp>
#include <kernel/cpuid.hpp>
#define __MM_MALLOC_H
#include <immintrin.h>


bool rdrand16(uint16_t* result)
{
  int res = 0;
  while (res == 0)
  {
    res = _rdrand16_step(result);
  }
  return (res == 1);
}

bool rdrand32(uint32_t* result)
{
  int res = 0;
  while (res == 0)
  {
    res = _rdrand32_step(result);
  }
  return (res == 1);
}
