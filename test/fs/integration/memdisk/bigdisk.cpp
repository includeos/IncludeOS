
#include <os>
#include <stdio.h>
#include <cassert>

#include <memdisk>

const uint64_t SIZE = 256000;

void Service::start(const std::string&)
{
  INFO("MemDisk", "Running tests for MemDisk");
  auto disk = fs::shared_memdisk();
  CHECKSERT(disk, "Created shared memdisk");

  CHECKSERT(not disk->empty(), "Disk is not empty");

  CHECKSERT(disk->dev().size() == SIZE, "Disk size is correct (%llu sectors)", SIZE);

  // read one block
  auto buf = disk->dev().read_sync(0);
  // verify nothing bad happened
  CHECKSERT(!!(buf), "Buffer for sector 0 is valid");

  INFO("MemDisk", "Verify MBR signature");
  const uint8_t* mbr = buf.get();
  CHECKSERT(mbr[0x1FE] == 0x55, "MBR has correct signature (0x1FE == 0x55)");
  CHECKSERT(mbr[0x1FF] == 0xAA, "MBR has correct signature (0x1FF == 0xAA)");

  INFO("MemDisk", "SUCCESS");
}
