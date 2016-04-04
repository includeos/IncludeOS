#include <sys/mman.h>

#include <cstdint>
#include <cstdlib>
#include <cassert>
#include <cerrno>
#include <map>

#define  MAP_FAILED  ((void*) -1)

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
           int prot,  int flags,
           int fd,    off_t offset)
{
  // invalid or misaligned length
  if (length == 0 || (length & 4095) != 0)
    {
      errno = EINVAL;
      return MAP_FAILED;
    }
  
  // associate some VA space with open file @fd
  // for now just allocate page-aligned on heap
  mmap_entry_t entry;
  entry.addr   = aligned_alloc(4096, length);
  entry.length = length;
  entry.prot   = prot;
  entry.flags  = flags;
  entry.fd     = fd;
  entry.offset = offset;
  
  /// Note: we may have to read a file here to properly create the
  /// in-memory mapping. Unfortunately, this would mean both mean
  /// mmap must be asynch, and also mean we have to implement Poshitx.
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
