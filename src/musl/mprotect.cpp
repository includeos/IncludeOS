#include "stub.hpp"
#include <kernel/memory.hpp>

static long sys_mprotect(void* addr, size_t len, int prot)
{
    if ((uintptr_t) addr & 0xFFF) {
        return -EINVAL;
    }
    if (len & 0xFFF) {
        return -EINVAL;
    }
    // TODO: mprotect(0x900000, 86016, 3) = -38 Function not implemented
    if (prot == 0x3) // read & write
    {
        // all the heap is already read/write
        return 0;
    }
    return -EINVAL;
}

extern "C"
long syscall_SYS_mprotect(void *addr, size_t len, int prot)
{
  return stubtrace(sys_mprotect, "mprotect", addr, len, prot);
}
