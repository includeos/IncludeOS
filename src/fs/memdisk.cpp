#include <fs/memdisk.hpp>

#include <cstring>
#include <cassert>
#include <cstdio>

extern "C"
{
  char _DISK_START_;
  char _DISK_END_;
}

namespace fs
{
  MemDisk::MemDisk()
  {
    
    image_start = &_DISK_START_;
    image_end   = &_DISK_END_;
  }
  
  void MemDisk::read_sector(block_t blk, on_read_func func)
  {
    static const size_t SECTOR_SIZE = 512;
    
    auto* sector_loc = ((char*) image_start) + blk * SECTOR_SIZE;
    // disallow reading memory past disk image
    assert(sector_loc < image_end);
    
    // copy block to new memory
    auto* buffer = new uint8_t[SECTOR_SIZE];
    assert( memcpy(buffer, sector_loc, SECTOR_SIZE) == buffer );
    
    // callback
    func(buffer);
  }
  
  void MemDisk::read_sectors(
      block_t start, 
      block_t count, 
      on_read_func func)
  {
    func(nullptr);
  }
  
}
