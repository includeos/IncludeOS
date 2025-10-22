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
#include <cstdio>
#include <hw/devices.hpp>
#include <fs/disk.hpp>
const uint64_t SIZE = 1*1024*1024;

#include "banana.ascii"
void Service::start()
{
  auto& device = hw::Devices::drive(0);

  INFO("IDE", "Running tests for writable IDE disk");
  // verify that the size is indeed N sectors
  CHECKSERT(device.size() == SIZE / 512, "Disk size is %lu sectors", SIZE / 512);

  static auto writebuf = fs::construct_buffer(1024);
  strncpy((char*) writebuf->data(), internal_banana.c_str(), writebuf->size());

  bool error = device.write_sync(0, writebuf);
  CHECKSERT(error == false, "Sync write success");
  printf("SUCCESS\n");
  return;

  device.write(0, writebuf,
  [] (const bool error) {
    CHECKSERT(!error, "Async write success");
    printf("SUCCESS\n");
  });


}
