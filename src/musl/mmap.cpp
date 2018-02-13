#include "common.hpp"
#include <cstdint>
#include <sys/mman.h>
#include <errno.h>
#include <os>
#include <util/alloc_lstack.hpp>

extern uintptr_t heap_begin;
extern uintptr_t heap_end = 0;
static uintptr_t current_pos = 0;

using Alloc = util::alloc::Lstack<4096>;
static Alloc alloc;

void init_mmap(uintptr_t addr_begin){
  Expects(alloc.empty());
  auto aligned_begin = (addr_begin + Alloc::align - 1) & ~(Alloc::align - 1);
  alloc.donate((void*)aligned_begin, (OS::heap_max() - aligned_begin) & ~(Alloc::align - 1));

}


extern "C" __attribute__((weak))
void* __kalloc(size_t size){
  return alloc.allocate(size);
}

extern "C" __attribute__((weak))
void __kfree (void* ptr, size_t size){
  alloc.deallocate(ptr, size);
}

static void* sys_mmap(void *addr, size_t length, int prot, int flags,
                      int fd, off_t offset)
{

  // TODO: Mapping to file descriptor
  if (fd > 0) {
    assert(false && "Mapping to file descriptor not yet implemented");
  }

  // TODO: mapping virtual address
  if (addr) {
    errno = ENODEV;
    return MAP_FAILED;
  }

  return __kalloc(length);;
}

extern "C"
void* syscall_SYS_mmap(void *addr, size_t length, int prot, int flags,
                      int fd, off_t offset)
{
  return strace(sys_mmap, "mmap", addr, length, prot, flags, fd, offset);
}

/**
  The mmap2() system call provides the same interface as mmap(2),
  except that the final argument specifies the offset into the file in
  4096-byte units (instead of bytes, as is done by mmap(2)).  This
  enables applications that use a 32-bit off_t to map large files (up
  to 2^44 bytes).

  http://man7.org/linux/man-pages/man2/mmap2.2.html
**/

extern "C"
void* syscall_SYS_mmap2(void *addr, size_t length, int prot,
                        int flags, int fd, off_t offset) {
  return strace(sys_mmap, "mmap2", addr, length, prot, flags, fd, offset * 4096);
}
