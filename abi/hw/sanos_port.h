#ifndef SANOS_PORT_H
#define SANOS_PORT_H

#include <common>

//From kmalloc.h
#define kmalloc(X) malloc(X)


//From klog.h
#define KERN_INFO "KERNEL:"
#define kprintf printf


//From types.h
#ifndef _BLKNO_T_DEFINED
#define _BLKNO_T_DEFINED
typedef unsigned int blkno_t;
#endif

typedef int port_t;
typedef int err_t;

#ifndef _HW_IO_DEFINED
#define _HW_IO_DEFINED


#ifndef _TID_T_DEFINED
#define _TID_T_DEFINED
typedef int tid_t;
#endif

#ifndef _HANDLE_T_DEFINED
#define _HANDLE_T_DEFINED
typedef int handle_t;
#endif


#ifndef _SIGSET_T_DEFINED
#define _SIGSET_T_DEFINED
typedef unsigned int sigset_t;
#endif


#ifndef _UID_T_DEFINED
#define _UID_T_DEFINED
typedef unsigned short uid_t;
#endif


#ifndef _GID_T_DEFINED
#define _GID_T_DEFINED
typedef unsigned short gid_t;
#endif


//From os.h
#define INFINITE  0xFFFFFFFF
#define MAXPATH 
#define NGROUPS_MAX 8
#define krnlapi 

//From pdir.h
#define PAGESHIFT      12
#define BTOP(x) ((unsigned long)(x) >> PAGESHIFT)
#define PAGESIZE 4096

//unsigned long virt2phys(void *vaddr);
//We don't need this, so just return the physical address
#define virt2phys(X) (uint32_t)X


//From trap.h

#define IRQBASE       0x20
#define IRQ2INTR(irq) (IRQBASE + (irq))

typedef int (*intrproc_t)(struct context *ctxt, void *arg);
void register_interrupt(struct interrupt *intr, int intrno, intrproc_t f, void *arg);


struct interrupt {
  struct interrupt *next;
  int flags;
  intrproc_t handler;
  void *arg;
};


//From sched.h
typedef void (*dpcproc_t)(void *arg);

void queue_irq_dpc(struct dpc *dpc, dpcproc_t proc, void *arg);

struct dpc {
  dpcproc_t proc;
  void *arg;
  struct dpc *next;
  int flags;
};




//From mach.h
/*
  In/Out - he uses a "machine object" with function pointers for these, wrapped in a bunch of aliases.
*/
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
