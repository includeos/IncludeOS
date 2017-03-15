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

#include <service>
#include <memdisk>
#include <os>
#include <fs/vfs.hpp>
#include <sys/stat.h>

int ftw_tests();
int stat_tests();

fs::Disk_ptr& memdisk() {
  static auto disk = fs::new_shared_memdisk();

  if (not disk->fs_ready()) {
    printf("%s\n", disk->name().c_str());
    disk->init_fs([](fs::error_t err, auto&) {
        if (err) {
          printf("ERROR MOUNTING DISK\n");
          exit(127);
        }
      });
    }
  return disk;
}

int main()
{
  INFO("POSIX stat", "Running tests for POSIX stat");

  // mount a disk with contents for testing
  auto root = memdisk()->fs().stat("/");
  fs::mount("/mnt/disk", root, "test root");

  fs::print_tree();

  stat_tests();
  ftw_tests();

  INFO("POSIX STAT", "All done!");

}
