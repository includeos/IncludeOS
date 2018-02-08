#include "common.hpp"
#include <string.h>
#include <kernel/os.hpp>
#include <errno.h>
#include <kprint>

__attribute__((weak))
uintptr_t __brk_max = 0x10000;

uintptr_t heap_begin = 0;

uintptr_t brk_end;
uintptr_t brk_init = 0;

extern void init_mmap(uintptr_t mmap_begin);

extern "C"
void _init_heap(uintptr_t free_mem_begin)
{
  #define HEAP_ALIGNMENT   4095
  // NOTE: Initialize the heap before exceptions
  // cache-align heap, because its not aligned
  heap_begin = free_mem_begin + HEAP_ALIGNMENT;
  heap_begin = ((uintptr_t)heap_begin & ~HEAP_ALIGNMENT);
  brk_end    = heap_begin;
  brk_init = brk_end;
  kprintf("* Brk initialized. Begin: 0x%lx, end 0x%lx\n",
          heap_begin, brk_end);

  init_mmap(heap_begin + __brk_max);

}


extern "C"
uintptr_t syscall_SYS_brk(void* addr)
{
  STRACE("syscall brk. Heap begin=0x%lx, OS::heap_max=0x%lx, addr=%p"
         "\n\t<brk> requests %i bytes \n",
         heap_begin, OS::heap_max(), addr, (uintptr_t)addr - brk_end);

  if (addr == nullptr
      or (uintptr_t)addr > heap_begin +  __brk_max
      or (uintptr_t)addr < heap_begin) {
    kprintf("\t<brk>Brk failed\n");
    return brk_end;
  }

  brk_end = (uintptr_t)addr;

  if (brk_end > brk_init) {
    kprintf("\t<brk> Initializing %i b\n", brk_end - brk_init);
    memset((void*)brk_init, 0, brk_end - brk_init);
    brk_init = brk_end;
  }

  kprintf("\t<brk> Done, returning 0x%lx\n", brk_end);
  return brk_end;
}
