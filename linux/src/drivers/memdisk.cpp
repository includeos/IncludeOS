#include <memdisk>

namespace fs {

static uint64_t stat;

MemDisk::MemDisk() noexcept
  : image_start_(0),
    image_end_(0),
    stat_read(stat)
{
  // ...
}

MemDisk::block_t MemDisk::size() const noexcept
{
  return 0;
}

buffer_t MemDisk::read_sync(block_t blk)
{
  return nullptr;
}
buffer_t MemDisk::read_sync(block_t blk, size_t cnt)
{
  return nullptr;
}

void MemDisk::deactivate() {}

} // fs
