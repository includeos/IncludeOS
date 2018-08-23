#include "common.hpp"
#include <string.h>
#include <kernel/os.hpp>
#include <errno.h>
#include <kprint>

static uintptr_t brk_begin        = 0;
static uintptr_t brk_end          = 0;
static uintptr_t brk_initialized  = 0;
extern ssize_t __brk_max;


uintptr_t __init_brk(uintptr_t begin)
{
  brk_begin = begin;
  brk_end   = begin;
  brk_initialized = brk_end;
  kprintf("* Brk initialized. Begin: %p, end %p, MAX %p\n",
          (void*) brk_begin, (void*) brk_end, (void*) __brk_max);
  return brk_begin + __brk_max;
}


size_t brk_bytes_used() {
  return brk_end - brk_begin;
}

size_t brk_bytes_free() {
  return __brk_max - brk_bytes_used();
}

static uintptr_t sys_brk(void* addr)
{
  if (addr == nullptr
      or (uintptr_t)addr > brk_begin +  __brk_max
      or (uintptr_t)addr < brk_begin) {
    return brk_end;
  }

  brk_end = (uintptr_t)addr;

  if (brk_end > brk_initialized) {
    memset((void*)brk_initialized, 0, brk_end - brk_initialized);
    brk_initialized = brk_end;
  }

  return brk_end;
}

extern "C"
uintptr_t syscall_SYS_brk(void* addr)
{
  return strace(sys_brk, "brk", addr);
}
