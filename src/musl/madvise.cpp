#include "common.hpp"
#include <sys/mman.h>

static long sys_madvise(void* addr, size_t length, int advice)
{
  switch (advice)
  {
    case MADV_NORMAL:
    case MADV_RANDOM:
    case MADV_SEQUENTIAL:
    case MADV_WILLNEED:
        return 0; // no-op
    case MADV_DONTNEED:
        // addr:len is unused, but the application hasn't freed the range
        return 0;
    case MADV_REMOVE:
    case MADV_FREE:
        printf("madvise with free/remove called on %p:%zu\n", addr, length);
        // TODO: free range
        return 0;
    default:
        printf("madvise with %d called on %p:%zu\n", advice, addr, length);
        return -EINVAL;
  }
}

extern "C"
long syscall_SYS_madvise(void* addr, size_t length, int advice) {
  return strace(sys_madvise, "madvise", addr, length, advice);
}
