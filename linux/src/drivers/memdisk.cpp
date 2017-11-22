#include <memdisk>
#include <cstdio>

namespace fs {

static uint64_t stat;

MemDisk::MemDisk() noexcept
  : image_start_(0),
    image_end_(0),
    stat_read(stat)
{
  auto* fp = fopen("memdisk.fat", "rb");
  assert(fp != nullptr && "Open memdisk.fat in source dir");
  fseek(fp, 0L, SEEK_END);
  int size = ftell(fp);
  rewind(fp);
  // read file into buffer
  char* buffer = new char[size];
  int res = fread(buffer, sizeof(char), size, fp);
  assert(res == size);
  // set image start/end
  image_start_ = buffer;
  image_end_   = buffer + size;
}

MemDisk::block_t MemDisk::size() const noexcept
{
  return image_end_ - image_start_;
}

buffer_t MemDisk::read_sync(block_t blk)
{
  return read_sync(blk, 1);
}
buffer_t MemDisk::read_sync(block_t blk, size_t cnt)
{
  if ((blk + cnt) * SECTOR_SIZE > size()) return nullptr;
  auto* start = &image_start_[blk * SECTOR_SIZE];
  auto* end   = start + cnt * SECTOR_SIZE;
  return fs::construct_buffer(start, end);
}

void MemDisk::deactivate() {
  delete[] image_start_;
}

} // fs
