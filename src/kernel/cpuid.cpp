#include <kernel/cpuid.hpp>
#include <cstring>

typedef CPUID::CPUIDinfo CPUIDinfo;

// EBX/RBX needs to be preserved depending on the memory model and use of PIC
static void
cpuid_info(CPUIDinfo *info, const unsigned int func, const unsigned int subfunc)
{
    __asm__ __volatile__ (
            "cpuid"
            : "=a"(info->EAX), "=b"(info->EBX), "=c"(info->ECX), "=d"(info->EDX)
            : "a"(func), "c"(subfunc)
    );
}

bool CPUID::isAmdCpu()
{
  CPUIDinfo info;
  cpuid_info(&info, 0, 0);
  if (memcmp((char *) (&info.EBX), "htuA", 4) == 0
   && memcmp((char *) (&info.EDX), "itne", 4) == 0
   && memcmp((char *) (&info.ECX), "DMAc", 4) == 0)
      return true;
  return false;
}

bool CPUID::isIntelCpu()
{
  CPUIDinfo info;
  cpuid_info(&info, 0, 0);
  if (memcmp((char *) (&info.EBX), "Genu", 4) == 0
   && memcmp((char *) (&info.EDX), "ineI", 4) == 0
   && memcmp((char *) (&info.ECX), "ntel", 4) == 0)
      return true;
  return false;
}

bool CPUID::hasRDRAND()
{
  if (!isAmdCpu() || !isIntelCpu())
    return false;
  
  CPUIDinfo info;
  cpuid_info(&info, 1, 0);
  
  static const unsigned int RDRAND_FLAG = (1 << 30);
  if ((info.ECX & RDRAND_FLAG) == RDRAND_FLAG)
    return true;
  
  return false;
}
