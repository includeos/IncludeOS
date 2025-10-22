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

// Includes std::string internal_banana
#include "banana.ascii"

void Service::start(const std::string&)
{
  INFO("FAT16", "Running tests for FAT16");
  auto disk = fs::shared_memdisk();
  assert(disk);

  // verify that the size is indeed N sectors
  CHECKSERT(disk->dev().size() == 6, "Disk size 6 sectors");

  // which means that the disk can't be empty
  CHECKSERT(!disk->empty(), "Disk not empty");

  // auto-init filesystem
  disk->init_fs(
  [] (fs::error_t err, fs::File_system& fs)
  {
    if (err) {
      printf("Init error: %s\n", err.to_string().c_str());
    }
    CHECKSERT(!err, "Filesystem auto-initialized");

    printf("\t\t%s filesystem\n", fs.name().c_str());

    auto list = fs.ls("/");
    CHECKSERT(!list.error, "List root directory");

    CHECKSERT(list.entries->size() == 3, "Exactly 3 entries in root dir");

    auto& e = list.entries->at(2);
    CHECKSERT(e.is_file(), "Ent is a file");
    CHECKSERT(e.name() == "banana.txt", "Ents name is 'banana.txt'");
  });
  // re-init on MBR (sigh)
  disk->init_fs(disk->MBR,
  [] (fs::error_t err, fs::File_system& fs)
  {
    CHECKSERT(!err, "Filesystem initialized on VBR1");

    // verify that we can read file
    auto ent = fs.stat("/banana.txt");
    CHECKSERT(ent.is_valid(), "Stat file in root dir");
    CHECKSERT(ent.is_file(), "Entity is file");
    CHECKSERT(!ent.is_dir(), "Entity is not directory");
    CHECKSERT(ent.name() == "banana.txt", "Name is 'banana.txt'");

    printf("Original banana (%zu bytes):\n%s\n",
            internal_banana.size(), internal_banana.c_str());

    // try reading banana-file
    auto buf = fs.read(ent, 0, ent.size());
    CHECKSERT(!buf.error(), "No error reading file");

    auto banana = buf.to_string();
    printf("New banana (%zu bytes):\n%s\n", banana.size(), banana.c_str());

    CHECKSERT(banana == internal_banana, "Correct banana #1");

    bool test = true;

    for (size_t i = 0; i < internal_banana.size(); i++)
    {
      // read one byte at a time
      buf = fs.read(ent, i, 1);
      /// @buf should evaluate to 'true' if its valid
      assert(buf);

      // verify that it matches the same location in test-string
      test = ((char) buf.data()[0] == internal_banana[i]);
      if (!test) {
        printf("!! Random access read test failed on i = %zu\n", i);
        break;
      }
    }
    CHECKSERT(test, "Validate random access sync read");

    buf = fs.read_file("/banana.txt");
    banana = buf.to_string();
    CHECKSERT(banana == internal_banana, "Correct banana #2");
  });

  // OK since we're using a memdisk (everything is synchronous)
  INFO("FAT16", "SUCCESS");
}
