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
  
  // verify that the size is indeed N sectors
  CHECK(disk->dev().size() == 16500, "Disk size 16500 sectors");
  assert(disk->dev().size() == 16500);
  
  // which means that the disk can't be empty
  CHECK(!disk->empty(), "Disk empty");
  assert(!disk->empty());
  
  // mount filesystem
  disk->mount(
  [disk] (fs::error_t err)
  {
    CHECK(!err, "Filesystem mounted");
    assert(!err);
    
    auto& fs = disk->fs();
    printf("\t\t%s filesystem\n", fs.name().c_str());
    
    auto vec = fs::new_shared_vector();
    err = fs.ls("/", vec);
    CHECK(!err, "List root directory");
    assert(!err);
    
    CHECK(vec->empty(), "Root directory is empty");
    assert(vec->empty());
  });
  
  INFO("MemDisk", "SUCCESS");
}
