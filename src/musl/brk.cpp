#include "common.hpp"
#include <string.h>
#include <kernel/os.hpp>
#include <errno.h>
#include <kprint>

__attribute__((weak))
uintptr_t __brk_max = 0x100000;

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

static uintptr_t sys_brk(void* addr)
{

  if (addr == nullptr
      or (uintptr_t)addr > heap_begin +  __brk_max
      or (uintptr_t)addr < heap_begin) {
    return brk_end;
  }

  brk_end = (uintptr_t)addr;

  if (brk_end > brk_init) {
    memset((void*)brk_init, 0, brk_end - brk_init);
    brk_init = brk_end;
  }

  return brk_end;
}


extern "C"
uintptr_t syscall_SYS_brk(void* addr)
{
  return strace(sys_brk, "brk", addr);
}
