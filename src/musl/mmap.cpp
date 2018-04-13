#include "common.hpp"
#include <cstdint>
#include <sys/mman.h>
#include <errno.h>
#include <util/alloc_lstack.hpp>
#include <os>
#include <kprint>

using Alloc = util::alloc::Lstack<4096>;
static Alloc alloc;

uintptr_t __init_mmap(uintptr_t addr_begin)
{
  Expects(alloc.empty());
  auto aligned_begin = (addr_begin + Alloc::align - 1) & ~(Alloc::align - 1);
  int64_t len = (OS::heap_max() - aligned_begin) & ~int64_t(Alloc::align - 1);
  alloc.donate((void*)aligned_begin, len);
  kprintf("* mmap initialized. Begin: 0x%zx, end: 0x%zx\n",
          addr_begin, addr_begin + len);
  return aligned_begin + len;
}

extern "C" __attribute__((weak))
void* kalloc(size_t size) {
  return alloc.allocate(size);
}

extern "C" __attribute__((weak))
void kfree (void* ptr, size_t size) {
  alloc.deallocate(ptr, size);
}

size_t mmap_bytes_used() {
  return alloc.bytes_allocated();
}

size_t mmap_bytes_free() {
  return alloc.bytes_free();
}

uintptr_t mmap_allocation_end(){
  return alloc.allocation_end();
}

static void* sys_mmap(void *addr, size_t length, int /*prot*/, int /*flags*/,
                      int fd, off_t /*offset*/)
{
  // TODO: Mapping to file descriptor
  if (fd > 0) {
    assert(false && "Mapping to file descriptor not yet implemented");
  }

  // TODO: mapping virtual address
  if (addr) {
    return MAP_FAILED;
  }

  return kalloc(length);
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
