// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
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

const uint64_t SIZE = 256000;

void Service::start(const std::string&)
{
  INFO("MemDisk", "Running tests for MemDisk");
  auto disk = fs::new_shared_memdisk();
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
