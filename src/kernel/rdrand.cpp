#include <kernel/rdrand.hpp>
#include <kernel/cpuid.hpp>
#include <cstring>

int RDRAND(uint8_t* buff, size_t bsize)
{
  if (!CPUID::hasRDRAND())
    return -1;
  
  size_t idx = 0, rem = bsize;
  size_t safety = bsize / sizeof(unsigned int) + 4;
  
  unsigned int val;
  while (rem > 0 && safety > 0)
  {
    char rc;    
    __asm__ volatile(
            "rdrand %0 ; setc %1"
            : "=r" (val), "=qm" (rc)
    );
    
    // 1 = success, 0 = underflow
    if (rc)
    {
      size_t cnt = (rem < sizeof(val) ? rem : sizeof(val));
      memcpy(buff + idx, &val, cnt);
      
      rem -= cnt;
      idx += cnt;
    }
    else
    {
      safety--;
    }
  }
  
  // Wipe temp on exit
  *((volatile unsigned int*)&val) = 0;
  
  // 0 = success; non-0 = failure (possibly partial failure).
  return (int)(bsize - rem);
}
