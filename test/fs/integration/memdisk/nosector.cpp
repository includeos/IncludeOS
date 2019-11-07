
#include <service>
#include <stdio.h>
#include <cassert>

#include <memdisk>

void Service::start(const std::string&)
{
  INFO("MemDisk", "Running tests for MemDisk");
  auto disk = fs::shared_memdisk();
  CHECKSERT(disk, "Created shared memdisk");

  // verify that the size is indeed 0 sectors
  CHECKSERT(disk->dev().size() == 0, "Disk size 0 sectors");

  // which means that the disk must be empty
  CHECKSERT(disk->empty(), "Disk empty");

  INFO("MemDisk", "SUCCESS");
}
