#ifndef HW_PCI_HPP
#define HW_PCI_HPP

#include <cstdint>

namespace hw {

  typedef uint16_t port_t;

  static inline int inp(port_t port)
  {
    int ret;
  
    __asm__ volatile("xorl %eax,%eax");
    __asm__ volatile("inb %%dx,%%al"
                     :"=a"(ret)
                     :"d"(port));
    return ret;  
  }

  static inline uint16_t inpw(port_t port)
  {
    uint16_t ret;
    __asm__ volatile("xorl %eax,%eax");
    __asm__ volatile("inw %%dx,%%ax"
                     :"=a"(ret)
                     :"d"(port));
    return ret;    
  }

  static inline uint32_t inpd(port_t port)
  {
    uint32_t ret;
    __asm__ volatile("xorl %eax,%eax");
    __asm__ volatile("inl %%dx,%%eax"
                     :"=a"(ret)
                     :"d"(port));
  
    return ret;
  }


  static inline void outp(port_t port, uint8_t data)
  {
    __asm__ volatile ("outb %%al,%%dx"::"a" (data), "d"(port));
  }
  static inline void outpw(port_t port, uint16_t data)
  {
    __asm__ volatile ("outw %%ax,%%dx"::"a" (data), "d"(port));
  }
  static inline void outpd(port_t port, uint32_t data)
  {
    __asm__ volatile ("outl %%eax,%%dx"::"a" (data), "d"(port));
  }

} //< namespace hw

#endif
