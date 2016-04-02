// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <os>
#include <stdio.h>
#include <cassert>

#include <memdisk>

void Service::start()
{
  INFO("MemDisk", "Running tests for MemDisk");
  auto disk = fs::new_shared_memdisk();
  assert(disk);
  
  CHECK((!disk->empty()) && (disk->dev().size() == 256000), "Correct disk size");
  // verify that the size is indeed 2 sectors
  assert(disk->dev().size() == 256000);
  // which means that the disk can't be empty
  assert(!disk->empty());
  
  // read one block
  auto buf = disk->dev().read_sync(0);
  // verify nothing bad happened
  CHECK(!!(buf), "Buffer for sector 0 is valid");
  if (!buf)
    {
      panic("Failed to read sector 0 on memdisk device\n");
    }
  // verify MBR has signature
  const uint8_t* mbr = buf.get();
  assert(mbr[0x1FE] == 0x55);
  assert(mbr[0x1FF] == 0xAA);
  
  INFO("MemDisk", "SUCCESS");
}
