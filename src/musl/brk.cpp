#include "common.hpp"
#include <string.h>
#include <os.hpp>
#include <errno.h>
#include <kprint>

static uintptr_t brk_begin        = 0;
static uintptr_t brk_end          = 0;
static uintptr_t brk_initialized  = 0;
static ssize_t   brk_max          = 0;


uintptr_t __init_brk(uintptr_t begin, size_t size)
{
  brk_begin = begin;
  brk_end   = begin;
  brk_max   = begin + size;
  brk_initialized = brk_end;
  return brk_begin + brk_max;
}


size_t brk_bytes_used() {
  return brk_end - brk_begin;
}

size_t brk_bytes_free() {
  return brk_max - brk_bytes_used();
}

static uintptr_t sys_brk(void* addr)
{
  if (addr == nullptr
      or (uintptr_t)addr > brk_begin +  brk_max
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
