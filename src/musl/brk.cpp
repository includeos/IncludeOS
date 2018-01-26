#include "common.hpp"
#include <string.h>
#include <kernel/os.hpp>
#include <errno.h>

uintptr_t heap_begin;
uintptr_t heap_end;
uintptr_t max_brk;

#warning stub

extern "C"
uintptr_t syscall_SYS_brk(void* addr)
{
  STRACE("syscall brk. Heap begin=0x%lx, OS::heap_max=0x%lx, addr=%p"
         "\n\t<brk> requests %i bytes \n",
         heap_begin, OS::heap_max(), addr, (uintptr_t)addr - heap_end);

  // Keep track of total used heap
  if (max_brk == 0)
    max_brk = heap_end;

  if (addr == nullptr
      or (uintptr_t)addr > OS::heap_max()
      or (uintptr_t)addr < heap_begin)
    return heap_end;

  heap_end = (uintptr_t)addr;

  if (heap_end > max_brk) {
    kprintf("\t<brk> Initializing %i b\n", heap_end - max_brk);
    memset((void*)max_brk, 0, heap_end - max_brk);
    max_brk = heap_end;
  }
  kprintf("\t<brk> Done, returning 0x%lx\n", heap_end);
  return heap_end;
}
