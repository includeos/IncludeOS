
#ifndef HW_IOPORT_HPP
#define HW_IOPORT_HPP

#include <common>
#include <arch.hpp>

namespace hw {

  static inline uint8_t inb(uint16_t port)
  {
    uint8_t ret;
#if defined(ARCH_x86)
    asm volatile("inb %1,%0" : "=a"(ret) : "Nd"(port));
#elif defined(ARCH_aarch64)
#else
#error "inp() not implemented for selected arch"
#endif
    return ret;
  }

  static inline uint16_t inw(uint16_t port)
  {
    uint16_t ret;
#if defined(ARCH_x86)
    asm volatile("inw %1,%0" : "=a"(ret) : "Nd"(port));
#elif defined(ARCH_aarch64)
#else
#error "inpw() not implemented for selected arch"
#endif
    return ret;
  }

  static inline uint32_t inl(uint16_t port)
  {
    uint32_t ret;
#if defined(ARCH_x86)
    asm volatile("inl %1,%0" : "=a"(ret) : "Nd"(port));
#elif defined(ARCH_aarch64)
#else
#error "inpd() not implemented for selected arch"
#endif
    return ret;
  }

  static inline void outb(uint16_t port, uint8_t data)
  {
#if defined(ARCH_x86)
    asm volatile ("outb %0,%1" :: "a"(data), "Nd"(port));
#elif defined(ARCH_aarch64)
#else
#error "outp() not implemented for selected arch"
#endif
  }
  static inline void outw(uint16_t port, uint16_t data)
  {
#if defined(ARCH_x86)
    asm volatile ("outw %0,%1" :: "a" (data), "Nd"(port));
#elif defined(ARCH_aarch64)
#else
#error "outpw() not implemented for selected arch"
#endif
  }
  static inline void outl(uint16_t port, uint32_t data)
  {
#if defined(ARCH_x86)
    asm volatile ("outl %0,%1" :: "a" (data), "Nd"(port));
#elif defined(ARCH_aarch64)
#else
#error "outpd() not implemented for selected arch"
#endif
  }

} //< namespace hw

#endif // HW_IOPORT_HPP
