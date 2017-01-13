// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2016-2017 Oslo and Akershus University College of Applied Sciences
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

#include <common.cxx>
#include <memdisk>

CASE("memdisk properties")
{
  auto disk = fs::new_shared_memdisk();
  EXPECT(disk->empty() == true);
  EXPECT(disk->device_id() == 0);
  EXPECT(disk->fs_ready() == false);
  EXPECT(disk->name() == "memdisk0");
  EXPECT(disk->dev().size() == 0ull);
  EXPECT(disk->dev().device_type() == "Block device");
  EXPECT(disk->dev().driver_name() == "MemDisk");
  bool enumerated_partitions {false};
  auto part_fn = [&enumerated_partitions](fs::error_t err, std::vector<fs::Disk::Partition>&) {
    if (!err)
      enumerated_partitions = true;
  };
  disk->partitions(part_fn);
  EXPECT(enumerated_partitions == true);
}

#include <hw/cpu.hpp>
CASE("...")
{
  uint64_t r1 = hw::CPU::rdtsc();
  uint64_t r2 = hw::CPU::rdtsc();
  EXPECT(r1 != r2);
}
