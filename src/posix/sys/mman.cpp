#include <sys/mman.h>

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cassert>
#include <cerrno>
#include <map>

struct mmap_entry_t
{
  void*  addr;
  size_t length;
  int    prot;
  int    flags;
  int    fd;
  off_t  offset;
};
std::map<void*, mmap_entry_t> _mmap_entries;

void* mmap(void* addr, size_t length,
           int  prot,  int flags,
           int  fd,    off_t offset)
{
  printf("mmap called with len=%u bytes\n", length);

  // invalid or misaligned length
  if (length == 0 || (length & 4095) != 0)
  {
    errno = EINVAL;
    return MAP_FAILED;
  }

  // TODO:
  // validate fd is open file

  // associate some VA space with open file @fd
  // for now just allocate page-aligned on heap
  mmap_entry_t entry;
  entry.addr   = aligned_alloc(4096, length);
  entry.length = length;
  entry.prot   = prot;
  entry.flags  = flags;
  entry.fd     = fd;
  entry.offset = offset;
  printf("mmap allocated %d bytes (%d pages)\n",
         length, length / 4096);

  // TODO:
  // read entire file into entry space
  // deallocate entry space on failure

  // create the mapping
  _mmap_entries[addr] = entry;
  return entry.addr;
}

int munmap(void* addr, size_t length)
{
  auto it = _mmap_entries.find(addr);
  if (it != _mmap_entries.end())
  {
    // <insert comment explaining why im doing this, or not>
    assert(it->second.length == length);

    // free and remove the entry
    free(it->second.addr);
    _mmap_entries.erase(it);
    return 0;
  }
  errno = EINVAL;
  return -1;
}

int mprotect(void *addr, size_t len, int prot)
{
  printf("mprotect %p:%u with flags %#x\n", addr, len, prot);
  return 0;
}
