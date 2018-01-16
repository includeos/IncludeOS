#include "common.hpp"
#include <cstdint>

extern uintptr_t heap_begin;
static uintptr_t current_pos = 0;
extern "C"
void* syscall_SYS_mmap(void *addr, size_t length, int prot, int flags,
                      int fd, off_t offset)
{
  uintptr_t res = heap_begin + current_pos;
  current_pos += length;
  STRACE("syscall mmap: addr=%p len=%u prot=%d fl=%d fd=%d off=%d\n",
    addr, length, prot, flags, fd, offset);
  return (void*)res;
}
