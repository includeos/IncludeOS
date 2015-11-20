#ifndef KERNEL_CPUID_HPP
#define KERNEL_CPUID_HPP

struct CPUID
{
  struct CPUIDinfo
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
