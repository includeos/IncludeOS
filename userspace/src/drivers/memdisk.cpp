#include <memdisk>
#include <cstdio>

namespace fs {

static uint64_t stat;

MemDisk::MemDisk() noexcept
  : stat_read(stat)
{
  auto* fp = fopen("memdisk.fat", "rb");
  assert(fp != nullptr && "Open memdisk.fat in source dir");
  fseek(fp, 0L, SEEK_END);
  long int size = ftell(fp);
  rewind(fp);
  //printf("MemDisk::MemDisk %zd size -> %zd sectors\n", size, size / SECTOR_SIZE);
  // read file into buffer
  char* buffer = new char[size];
  size_t res = fread(buffer, size, 1, fp);
  assert(res == 1);
  // set image start/end
  image_start_ = buffer;
  image_end_   = buffer + size;
}

MemDisk::block_t MemDisk::size() const noexcept
{
  return (image_end_ - image_start_) / SECTOR_SIZE;
}

buffer_t MemDisk::read_sync(block_t blk, size_t cnt)
{
  //printf("MemDisk::read %zu -> %zu / %zu\n", blk, blk + cnt, size() / SECTOR_SIZE);
  if (blk + cnt > size()) return nullptr;
  auto* start = &image_start_[blk * SECTOR_SIZE];
  return fs::construct_buffer(start, start + cnt * SECTOR_SIZE);
}

void MemDisk::deactivate() {
  delete[] image_start_;
}

} // fs
