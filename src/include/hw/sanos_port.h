#ifndef SANOS_PORT_H
#define SANOS_PORT_H

#include <os>

#ifndef _BLKNO_T_DEFINED
#define _BLKNO_T_DEFINED
typedef unsigned int blkno_t;
#endif

typedef int port_t;

#ifndef _HW_IO_DEFINED
#define _HW_IO_DEFINED

static inline int inp(port_t port){
  int ret;
  
  __asm__ volatile("xorl %eax,%eax");
  __asm__ volatile("inb %%dx,%%al"
		   :"=a"(ret)
		   :"d"(port));
  return ret;  
}

static inline uint16_t inpw(port_t port){
  uint16_t ret;
  __asm__ volatile("xorl %eax,%eax");
  __asm__ volatile("inw %%dx,%%ax"
		   :"=a"(ret)
		   :"d"(port));
  return ret;    
}

static inline uint32_t inpd(port_t port){
  uint32_t ret;
  __asm__ volatile("xorl %eax,%eax");
  __asm__ volatile("inl %%dx,%%eax"
		   :"=a"(ret)
		   :"d"(port));
  
  return ret;
}


static inline void outp(port_t port, uint8_t data){
  __asm__ volatile ("outb %%al,%%dx"::"a" (data), "d"(port));
}
static inline void outpw(port_t port, uint16_t data){
  __asm__ volatile ("outw %%ax,%%dx"::"a" (data), "d"(port));
}
static inline void outpd(port_t port, uint32_t data){
  __asm__ volatile ("outl %%eax,%%dx"::"a" (data), "d"(port));
}


#endif
#endif
