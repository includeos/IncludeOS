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
std::shared_ptr<fs::Disk> disk;

std::string internal_banana = 
  R"(     ____                           ___
    |  _ \  ___              _   _.' _ `.
 _  | [_) )' _ `._   _  ___ ! \ | | (_) |    _
|:;.|  _ <| (_) | \ | |' _ `|  \| |  _  |  .:;|
|   `.[_) )  _  |  \| | (_) |     | | | |.',..|
':.   `. /| | | |     |  _  | |\  | | |.' :;::'
 !::,   `-!_| | | |\  | | | | | \ !_!.'   ':;!
 !::;       ":;:!.!.\_!_!_!.!-'-':;:''    '''!
 ';:'        `::;::;'             ''     .,  .
      `:     .,.    `'    .::... .      .::;::;'
      `..:;::;:..      ::;::;:;:;,    :;::;'
       "-:;::;:;:      ':;::;:''     ;.-'
           ""`---...________...---'""
       )";

void Service::start()
{
  INFO("FAT32", "Running tests for FAT32");
  auto& device = hw::Dev::disk<0, hw::IDE>(hw::IDE::SLAVE);
  disk = std::make_shared<fs::Disk> (device);
  assert(disk);
  
  // verify that the size is indeed N sectors
  const size_t SIZE = 4194304;
  printf("Size: %llu\n", disk->dev().size());
  CHECKSERT(disk->dev().size() == SIZE, "Disk size 4194304 sectors");
  
  // which means that the disk can't be empty
  CHECKSERT(!disk->empty(), "Disk not empty");
  
  // auto-mount filesystem
  disk->mount(disk->MBR,
  [] (fs::error_t err)
  {
    CHECKSERT(!err, "Filesystem auto-mounted");
    
    auto& fs = disk->fs();
    printf("\t\t%s filesystem\n", fs.name().c_str());
    
    auto vec = fs::new_shared_vector();
    err = fs.ls("/", vec);
    CHECKSERT(!err, "List root directory");
    
    CHECKSERT(vec->size() == 2, "Exactly two ents in root dir");
    
    auto& e = vec->at(0);
    CHECKSERT(e.is_file(), "Ent is a file");
    CHECKSERT(e.name() == "banana.txt", "Ents name is 'banana.txt'");
  });
  // re-mount on VBR1
  disk->mount(disk->VBR1,
  [] (fs::error_t err)
  {
    CHECK(!err, "Filesystem mounted on VBR1");
    assert(!err);
    
    auto& fs = disk->fs();
    auto ent = fs.stat("/banana.txt");
    CHECKSERT(ent.is_valid(), "Stat file in root dir");
    CHECKSERT(ent.is_file(), "Entity is file");
    CHECKSERT(!ent.is_dir(), "Entity is not directory");
    CHECKSERT(ent.name() == "banana.txt", "Name is 'banana.txt'");
    
    ent = fs.stat("/dir1/dir2/dir3/dir4/dir5/dir6/banana.txt");
    CHECKSERT(ent.is_valid(), "Stat file in deep dir");
    CHECKSERT(ent.is_file(), "Entity is file");
    CHECKSERT(!ent.is_dir(), "Entity is not directory");
    
    CHECKSERT(ent.name() == "banana.txt", "Name is 'banana.txt'");
    
    printf("%s\n", internal_banana.c_str());
    
    // asynch file reading test
    fs.read(ent, 0, ent.size,
    [&fs] (fs::error_t err, fs::buffer_t buf, uint64_t len)
    {
      CHECKSERT(!err, "read: Read 'banana.txt' asynchronously");
      if (err)
      {
        panic("Failed to read file async");
      }
      
      std::string banana((char*) buf.get(), len);
      CHECKSERT(banana == internal_banana, "Correct banana #1");
      
      fs.readFile("/banana.txt",
      [&fs] (fs::error_t err, fs::buffer_t buf, uint64_t len)
      {
        CHECKSERT(!err, "readFile: Read 'banana.txt' asynchronously");
        if (err)
        {
          panic("Failed to read file async");
        }
        
        std::string banana((char*) buf.get(), len);
        CHECKSERT(banana == internal_banana, "Correct banana #2");
      });
    });
  });
  
  INFO("FAT32", "SUCCESS");
}
