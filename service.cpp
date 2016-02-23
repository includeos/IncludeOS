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
#include <net/inet4>
#include <net/dhcp/dh4client.hpp>
#include <http>

// An IP-stack object
std::unique_ptr<net::Inet4<VirtioNet> > inet;

#include <memdisk>
#include <fs/fat.hpp> // FAT32 filesystem
using namespace fs;

// assume that devices can be retrieved as refs with some kernel API
// for now, we will just create it here
MemDisk device;

// describe a disk with FAT32 mounted on partition 0 (MBR)
using MountedDisk = fs::Disk<FAT32>;
// disk with filesystem
std::unique_ptr<MountedDisk> disk;

void Service::start() {
  hw::Nic<VirtioNet>& eth0 = hw::Dev::eth<0,VirtioNet>();
  inet = std::make_unique<net::Inet4<VirtioNet> >(eth0);
  
  inet->network_config( {{ 10,0,0,42 }},      // IP
			{{ 255,255,255,0 }},  // Netmask
			{{ 10,0,0,1 }},       // Gateway
			{{ 8,8,8,8 }} );      // DNS


  ////// DISK //////
  // instantiate disk with filesystem
  disk = std::make_unique<MountedDisk> (device);
  
  // mount the main partition in the Master Boot Record
  disk->mount(MountedDisk::MBR,
  [] (fs::error_t err)
  {
    
    if (err)
    {
      printf("Could not mount filesystem\n");
      return;
    }

    ///// TCP SERVER ///
    auto& server = inet->tcp().bind(80);
    
    printf("<Service> Added listener: %s \n", server.to_string().c_str());
    
    server.onReceive([](auto conn, bool push) {
      using namespace http;
      bool buffer_full = !push;

      Request req{conn->read(1024)};
      printf("URI: %s \n", req.get_uri().c_str());
      Response res;

      conn->write(res);
    });  

  });



  
}
