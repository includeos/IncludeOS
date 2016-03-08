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

#include <fs/fat.hpp>
#include <ide>
using FatDisk = fs::Disk<fs::FAT>;
std::shared_ptr<FatDisk> disk;

void Service::start()
{
  INFO("FAT32", "Running tests for FAT32");
  auto& device = hw::Dev::disk<0, hw::IDE>(hw::IDE::SLAVE);
  disk = std::make_shared<FatDisk> (device);
  assert(disk);
  
  // verify that the size is indeed N sectors
  printf("Disk size: %lu ???\n", disk->dev().size());
  //const size_t SIZE = 4194304;
  //CHECK(disk->dev().size() == SIZE, "Disk size 4194304 sectors");
  //assert(disk->dev().size() == SIZE);
  
  // which means that the disk can't be empty
  CHECK(!disk->empty(), "Disk not empty");
  assert(!disk->empty());
  
  // auto-mount filesystem
  disk->mount(disk->MBR,
  [] (fs::error_t err)
  {
    CHECK(!err, "Filesystem auto-mounted");
    assert(!err);
    
    auto& fs = disk->fs();
    printf("\t\t%s filesystem\n", fs.name().c_str());
    
    auto vec = fs::new_shared_vector();
    err = fs.ls("/", vec);
    CHECK(!err, "List root directory");
    assert(!err);
    
    CHECK(vec->size() == 1, "Exactly one ent in root dir");
    assert(vec->size() == 1);
    
    auto& e = vec->at(0);
    CHECK(e.is_file(), "Ent is a file");
    CHECK(e.name() == "Makefile", "Ent is 'Makefile'");
  });
  // re-mount on VBR1
  disk->mount(disk->VBR1,
  [] (fs::error_t err)
  {
    CHECK(!err, "Filesystem mounted on VBR1");
    assert(!err);
    
    auto& fs = disk->fs();
    auto ent = fs.stat("/Makefile");
    CHECK(ent.is_valid(), "Stat file in root dir");
    CHECK(ent.is_file(), "Entity is file");
    CHECK(!ent.is_dir(), "Entity is not directory");
    CHECK(ent.name() == "Makefile", "Name is 'Makefile'");
  });
  
  INFO("FAT32", "SUCCESS");
}
