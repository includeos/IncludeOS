#include "common.hpp"
#include "util/errno.hpp"
#include <cstdint>
#include <sys/mman.h>
#include <errno.h>
#include <os>
#include <mem/alloc.hpp>
#include <mem/allocator.hpp>
#include "mem/flags.hpp"
#include <kernel.hpp>
#include <kprint>

using Alloc = os::mem::Raw_allocator;
static Alloc* default_allocator;

os::mem::Raw_allocator& os::mem::raw_allocator() {
  Expects(default_allocator);
  return *default_allocator;
}

uintptr_t __init_mmap(uintptr_t addr_begin, size_t size)
{
  auto aligned_begin = (addr_begin + Alloc::align - 1) & ~(Alloc::align - 1);
  int64_t len = size & ~int64_t(Alloc::align - 1);

  default_allocator = Alloc::create((void*)aligned_begin, len);
  return aligned_begin + len;
}

extern "C" __attribute__((weak))
void* kalloc(size_t size) {
  Expects(kernel::heap_ready());
  return default_allocator->allocate(size);
}

extern "C" __attribute__((weak))
void* kalloc_aligned(size_t alignment, size_t size) {
  Expects(kernel::heap_ready());
  return default_allocator->do_allocate(size, alignment);
}

extern "C" __attribute__((weak))
void kfree (void* ptr, size_t size) {
  default_allocator->deallocate(ptr, size);
}

size_t mmap_bytes_used() {
  return default_allocator->bytes_used();
}

size_t mmap_bytes_free() {
  return default_allocator->bytes_free();
}

uintptr_t mmap_allocation_end() {
  return default_allocator->highest_used();
}

static void *mmap_failed(int _errno) {
  kprintf("[mmap] FAIL errno=%s free=%zu used=%zu\n", util::format::errno_cstr(_errno),
          mmap_bytes_free(), mmap_bytes_used());

  errno = _errno;
  return MAP_FAILED;
}

using Fd = std::optional<int>;  // FIXME: proper type

static void* __sys_mmap(/*const*/ uintptr_t addr, const size_t length, const os::mem::Access prot,
                        const os::mem::alloc::Flags flags, const Fd file_descriptor, const off_t offset)
{
  using namespace os::mem::alloc;
  // NOTE: `must` and `should` messages in this function refer to POSIX mmap(3p)

  // kprintf("[mmap] addr=%p len=%zu prot=0x%x flags=0x%x fd=%d off=%lld free=%zu used=%zu\n", _addr, length, prot, _flags, _fd, (long long)offset, mmap_bytes_free(), mmap_bytes_used());  // TODO: debugging

  // TODO(mazunki): this is unnecessary when ::Sharing becomes a real type
  if (util::has_flag(flags, Sharing::Private) == util::has_flag(flags, Sharing::Shared)) {
    Expectsf(false, "sys_mmap: mapping must be either Private xor Shared");
    return mmap_failed(EINVAL);
  }

  if (length == 0) {
    Expectsf(false, "Mapping must never allocate 0 bytes");
      return mmap_failed(EINVAL);
  }

  if (util::has_flag(Flags::Anonymous)) {
    if (file_descriptor) {
      Expectsf(false, "Anonymous mappings must set fd=-1, got fd={}", file_descriptor.value());  // TODO(mazunki): rename -1 when signature changes
      return mmap_failed(EINVAL);
    }
    if (offset != 0) {
      Expectsf(false, "Anonymous mappings should have offset=0, got offset={}", offset);
      return mmap_failed(EINVAL);
    }
  }

  // NOTE: specifying an address with non-fixed allocation is, per POSIX, only
  // a hint. for now, we ignore this hint.
  //
  // this is the only reason why `uintptr_t addr` isn't const
  if ((addr != 0) && util::missing_flag(flags, Flags::FixedOverride))  {
    addr = 0;
  }

  // TODO: Implement minimal functionality to be POSIX compliant
  // https://pubs.opengroup.org/onlinepubs/009695399/functions/mmap.html

  if (file_descriptor) {
    Expectsf(false, "Mapping to file descriptor is not yet implemented, got fd={}", file_descriptor.value());
    return mmap_failed(ENOTSUP);
  }

  if (util::missing_flag(flags, Sharing::Anonymous)) {
    Expectsf(false, "Support for non-MAP_ANONYMOUS mappings is not yet implemented");
    return mmap_failed(ENOTSUP);
  }

  void *res = nullptr;

  if (util::has_flag(flags, Flags::FixedOverride) or util::has_flag(flags, Flags::FixedFriendly)) {
    if (addr == 0) {
      return mmap_failed(EINVAL); // invalid address
    }
    if ((addr % os::mem::PAGE_SZ) != 0) {
      return mmap_failed(ENOTSUP); // invalid alignment
    }

    const size_t len_rounded = util::bits::roundto(os::mem::PAGE_SZ, length);
    const bool do_override = util::has_flag(flags, Flags::FixedOverride) ? true : false;

    return mmap_failed(ENOTSUP);
    // res = kalloc_fixed(reinterpret_cast<void*>(addr), len_rounded, do_override);

  } else {
    if (util::has_flag(flags, Flags::Private)) {
      if (util::missing_flag(flags, Flags::Anonymous)) {
        Expectsf(false, "Support for MAP_PRIVATE other than with MAP_ANONYMOUS is not yet implemented");
        return mmap_failed(ENOTSUP);
      }
      if (addr != 0) {
        Expectsf(false, "Support for MAP_PRIVATE other than for new allocations (addr=0) is not yet implemented. Got addr={}", addr);
        return mmap_failed(ENOTSUP);
      }
    }

    res = kalloc(length);
  }

  if (UNLIKELY(res == nullptr)) {
    return mmap_failed(ENOMEM);
  }

  memset(res, 0, length);
  return res;
}

static void* sys_mmap(void *_addr, size_t length, int _prot, int _flags, int _fd, off_t offset)
{
  const os::mem::alloc::Flags flags = static_cast<os::mem::alloc::Flags>(_flags);
  const os::mem::alloc::Protection prot = static_cast<os::mem::alloc::Protection>(_prot);
  const Fd file_descriptor = (_fd == -1) ? std::nullopt : Fd(_fd);
  const uintptr_t addr = reinterpret_cast<uintptr_t>(_addr);

  return __sys_mmap(addr, length, prot, flags, file_descriptor, offset);
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
