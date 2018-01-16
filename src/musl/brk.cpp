#include "common.hpp"
#include <cstdint>
#include <kernel/os.hpp>
#include <errno.h>

uintptr_t heap_begin;
uintptr_t heap_end;

extern "C"
int syscall_SYS_brk(void* end_data_segment)
{
  uintptr_t actual = heap_begin + (uintptr_t)end_data_segment;
  int res = actual <= OS::heap_max() ? 0 : -1;

  if(res == 0)
  {
    heap_end = actual;
  }

  STRACE("syscall brk(end_data_seg=%p) = %d\n",
    end_data_segment, res);

  return res;
}
