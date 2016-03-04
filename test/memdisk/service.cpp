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
  auto disk = fs::new_shared_memdisk();
  // verify that the size is indeed 2 sectors
  assert(disk->dev().size() == 2);
  printf("[x] Correct disk size\n");
  // read one block
  auto buf = disk->dev().read_sync(0);
  // verify nothing bad happened
  if (!buf)
  {
    panic("Failed to read sector 0 on memdisk device\n");
  }
  printf("[x] Buffer for sector 0 contains is valid\n");
  // convert to text
  std::string text((const char*) buf.get(), disk->dev().block_size());
  // verify that the sector contents matches the test string
  // NOTE: the 3 first characters are non-text 0xBFBBEF
  std::string test1 = "\xEF\xBB\xBFThe Project Gutenberg EBook of Pride and Prejudice, by Jane Austen";
  std::string test2 = text.substr(0, test1.size());
  assert(test1 == test2);
  printf("[x] Binary comparison of sector data\n");
  // verify that reading outside of disk returns a 0x0 pointer
  buf = disk->dev().read_sync(disk->dev().size());
  assert(!buf);
  printf("[x] Buffer outside of disk range (sector=%u) is 0x0\n",
      disk->dev().size());
}
