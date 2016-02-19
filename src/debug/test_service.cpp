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
#include <net/inet4>
#include <math.h>
#include <iostream>
#include <sstream>
#include <net/dhcp/dh4client.hpp>

using namespace std::chrono;

// An IP-stack object
std::unique_ptr<net::Inet4<VirtioNet> > inet;

// our VGA output module
#include <kernel/vga.hpp>
ConsoleVGA vga;

// IDE
#include <hw/ide.hpp>
#include <fs/disk.hpp>
#include <fs/fat.hpp>

void Service::start()
{
  /// virtio-console testing ///
  /*
  auto con = hw::Dev::console<0, VirtioCon> ();
  
  // set secondary serial output to VGA console module
  printf("Attaching console...\n");
  OS::set_rsprint(
  [&con] (const char* data, size_t len)
  {
    con.write(data, len);
    //for (size_t i = 0; i < len; i++)
    //    OS::rswrite(data[i]);
  });*/
  
  /*
  const char* test = "Testing :(\n";
  size_t L = strlen(test);
  
  for (int i = 0; i < 120; i++)
      con.write(test, L);
  */
  
  // Assign a driver (VirtioNet) to a network interface (eth0)
  // @note: We could determine the appropirate driver dynamically, but then we'd
  // have to include all the drivers into the image, which  we want to avoid.
  hw::Nic<VirtioNet>& eth0 = hw::Dev::eth<0,VirtioNet>();
  
  // Bring up a network stack, attached to the nic
  inet = std::make_unique<net::Inet4<VirtioNet> >(eth0);
  
  // Static IP configuration, until we (possibly) get DHCP
  // @note : Mostly to get a robust demo service that it works with and without DHCP
  inet->network_config(
      {{ 10,0,0,42 }},      // IP
			{{ 255,255,255,0 }},  // Netmask
			{{ 10,0,0,1 }},       // Gateway
			{{ 8,8,8,8 }} );      // DNS
  
  
  /// PCI IDE controller testing ///
  using FatDisk = fs::Disk<fs::FAT32>;
  
  auto ide1 = hw::Dev::disk<0, hw::IDE> ( /** probably need some option here **/ );
  auto disk = std::make_shared<FatDisk> (ide1);
  
  ide1.read_sector(0, 
  [] (const void* data)
  {
    auto* mbr = (fs::MBR::mbr*) data;
    
    printf("OEM name: %.8s\n", mbr->oem_name);
    printf("MAGIC sig: 0x%x\n", mbr->magic);
  });
  
  disk->mount(FatDisk::MBR,
  [] (fs::error_t err)
  {
    if (err)
    {
      printf("BAD\n");
      return;
    }
    
    printf("GOOD ?\n");
    
  });
  
  printf("*** TEST SERVICE STARTED *** \n");
}
