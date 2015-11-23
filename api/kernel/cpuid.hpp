#ifndef KERNEL_CPUID_HPP
#define KERNEL_CPUID_HPP

struct CPUID
{
  struct cpuid_t
  {
    unsigned int EAX;
    unsigned int EBX;
    unsigned int ECX;
    unsigned int EDX;
  };
  
  static bool isAmdCpu();
  static bool isIntelCpu();
  static bool hasRDRAND();
};

#endif
