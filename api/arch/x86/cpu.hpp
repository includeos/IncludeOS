
#pragma once
#ifndef X86_CPU_HPP
#define X86_CPU_HPP

#include <cstdint>
#include <arch.hpp>

#define IA32_EFER               0xC0000080
#define IA32_STAR               0xC0000081
#define IA32_LSTAR              0xc0000082
#define IA32_FMASK              0xc0000084
#define IA32_FS_BASE            0xC0000100
#define IA32_GS_BASE            0xC0000101
#define IA32_KERNEL_GS_BASE     0xC0000102

namespace x86
{

  class CPU
  {
  public:
    // read the intel manual
    static uint64_t
    read_msr(uint32_t addr)
    {
#if defined(__x86_64__)
        uint32_t low, high;
        asm volatile (
          "rdmsr"
          : "=a"(low), "=d"(high)
          : "c"(addr)
        );
        return ((uint64_t)high << 32) | low;

#elif defined(__i386__)
      uint64_t v;
      asm volatile("rdmsr" : "=A" (v) : "c" (addr));
      return v;
#else
#error "read_msr() not implemented for selected arch"
#endif
    }

    static void
    write_msr(uint32_t addr, uint32_t eax, uint32_t edx)
    {
#if defined(__x86_64__) || defined(__i386__)
      asm volatile("wrmsr" : : "a" (eax), "d"(edx), "c" (addr));
#else
#error "write_msr() not implemented for selected arch"
#endif
    }

    static void
    write_msr(uint32_t addr, uint64_t value)
    {
#if defined(__x86_64__)
      const uint32_t eax = value & 0xffffffff;
      const uint32_t edx = value >> 32;
      asm volatile("wrmsr" : : "a" (eax), "d"(edx), "c" (addr));
#elif defined(__i386__)
      asm volatile("wrmsr" : : "A" (value), "c" (addr));
#else
#error "write_msr() not implemented for selected arch"
#endif
    }

#if defined(__x86_64__)
    static void set_fs(void* entry) noexcept {
      write_msr(IA32_FS_BASE, (uintptr_t) entry);
    }
    static void set_gs(void* entry) noexcept {
      write_msr(IA32_GS_BASE, (uintptr_t) entry);
    }
#endif

  };
}

#endif
