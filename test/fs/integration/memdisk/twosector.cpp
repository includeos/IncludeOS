
#include <service>
#include <stdio.h>
#include <cassert>
#include <common>

#include <memdisk>

void Service::start(const std::string&)
{
  INFO("MemDisk", "Running tests for MemDisk");
  auto disk = fs::shared_memdisk();
  CHECKSERT(disk, "Created shared memdisk");

  CHECKSERT(not disk->empty(), "Disk is not empty");
  // verify that the size is indeed 2 sectors
  CHECKSERT(disk->dev().size() == 2, "Disk size is correct (2 sectors)");

  // read one block
  auto buf = disk->dev().read_sync(0);
  // verify nothing bad happened
  CHECKSERT(!!buf, "Buffer for sector 0 is valid");

  // convert to text (before reading sector 2)
  std::string text((const char*) buf->data(), buf->size());

  // read another block
  buf = disk->dev().read_sync(1);
  // verify nothing bad happened
  CHECKSERT(buf != nullptr, "Buffer for sector 1 is valid");

  // verify that the sector contents matches the test string
  // NOTE: the 3 first characters are non-text 0xBFBBEF
  std::string test1 = "\xEF\xBB\xBFThe Project Gutenberg EBook of Pride and Prejudice, by Jane Austen";
  std::string test2 = text.substr(0, test1.size());
  CHECKSERT(test1 == test2, "Binary comparison of sector data");

  // verify that reading outside of disk returns a 0x0 pointer
  buf = disk->dev().read_sync(disk->dev().size());
  CHECKSERT(buf == nullptr, "Buffer outside of disk range (sector=%llu) is 0x0",
            disk->dev().size());

  INFO("MemDisk", "SUCCESS");
}
