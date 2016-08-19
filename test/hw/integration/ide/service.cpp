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
#include <fs/disk.hpp>
#include <fs/mbr.hpp>
#include <fs/fat.hpp>
#include <hw/ide.hpp>

void Service::start(const std::string&)
{
  printf("TESTING Disks\n\n");

  auto ide = hw::Dev::disk<0, hw::IDE>(hw::IDE::MASTER);

  printf("Name : %s\n", ide.name());
  printf("Size : %llu\n\n", ide.size());

  printf("Reading sync:\n");
  auto* mbr = (fs::MBR::mbr*)ide.read_sync(0).get();
  printf("Name: %.8s\n", mbr->oem_name);
  printf("MAGIC sig: 0x%x\n\n", mbr->magic);

  ide.read(0, 3, [] (hw::IDE::buffer_t data) {
      static int i = 0;
      uint8_t* buf = (uint8_t*)data.get();
      printf("Async read, Block %d:\n", i);
      for (int i = 0; i < 512; i++)
        printf("%x ", buf[i]);
      printf("\n");
      i++;
    });

  printf("Reading sync:\n");
  mbr = (fs::MBR::mbr*)ide.read_sync(0).get();
  printf("Name: %.8s\n", mbr->oem_name);
  printf("MAGIC sig: 0x%x\n\n", mbr->magic);

  ide.read(4, [] (hw::IDE::buffer_t data) {
      uint8_t* buf = (uint8_t*)data.get();
      printf("Async read, Block %d:\n", 4);
      for (int i = 0; i < 512; i++)
        printf("%x ", buf[i]);
      printf("\n");
    });
}
