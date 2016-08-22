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
//1 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#define OS_TERMINATE_ON_CONTRACT_VIOLATION
#include <os>

#include <fs/disk.hpp>
std::shared_ptr<fs::Disk> disk;

void list_partitions(decltype(disk));

#define MYINFO(X,...) INFO("VirtioBlk",X,##__VA_ARGS__)

void Service::start(const std::string&)
{
  // instantiate memdisk with FAT filesystem
  auto& device = hw::Devices::drive(0);
  disk = std::make_shared<fs::Disk> (device);
  // assert that we have a disk
  CHECKSERT(disk, "Disk created");
  // if the disk is empty, we can't mount a filesystem
  CHECKSERT(not disk->empty(), "Disk is not empty");

  // list extended partitions
  list_partitions(disk);

  // mount first valid partition (auto-detect and mount)
  disk->mount(
  [] (fs::error_t err) {
    if (err) {
      printf("Could not mount filesystem\n");
      panic("mount() failed");
    }

    // async ls
    disk->fs().ls("/",
    [] (fs::error_t err, auto ents) {
      if (err) {
        printf("Could not list '/' directory\n");
        panic("ls() failed");
      }

      // go through directory entries
      for (auto& e : *ents) {
        printf("%s: %s\t of size %llu bytes (CL: %llu)\n",
               e.type_string().c_str(), e.name().c_str(), e.size(), e.block);

        if (e.is_file()) {
          printf("*** Read %s\n", e.name().c_str());
          disk->fs().read(e, 0, e.size(),
          [e] (fs::error_t err, fs::buffer_t buffer, size_t len) {
            if (err) {
              printf("Failed to read %s!\n", e.name().c_str());
              panic("read() failed");
            }

            std::string contents((const char*) buffer.get(), len);
            printf("[%s contents]:\n%s\nEOF\n\n",
                   e.name().c_str(), contents.c_str());
            // ---
            INFO("Virtioblk Test", "SUCCESS");
          });

        } // is_file

      } // ents

    }); // ls

  }); // disk->auto_detect()

  printf("*** TEST SERVICE STARTED *** \n");

}

void list_partitions(decltype(disk) disk)
{
  disk->partitions([] (fs::error_t err, auto& parts) {

      CHECKSERT (not err, "Was able to fetch partition table");

      for (auto& part : parts)
        printf("[Partition]  '%s' at LBA %u\n",
               part.name().c_str(), part.lba());

    });
}
