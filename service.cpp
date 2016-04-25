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
#include <http/request.hpp>
#include <http/response.hpp>
#include <fs/disk.hpp>
#include <cassert>

namespace fs {
  typedef FileSystem::Dirent Dirent;
}
namespace net {
  typedef TCP::Connection_ptr Connection_ptr;
}

void send_status(net::Connection_ptr conn, fs::Disk_ptr, int code)
{
  http::Response response(code);
  std::string data(response.to_string());
  
  conn->write(data.data(), data.size());
}
void send_file(net::Connection_ptr conn, fs::Disk_ptr disk, const fs::Dirent& dirent)
{
   disk->fs().read(dirent, 0, dirent.size,
   [conn, disk, dirent] (fs::error_t err, fs::buffer_t buffer, size_t size) {
     if (err) {
       // internal error
       send_status(conn, disk, 500);
     }
     // send file
     conn->write(buffer.get(), size);
   });
}
void send_directory(net::Connection_ptr, fs::Disk_ptr disk, const fs::Dirent& dirent)
{
  
}

void client_read(
    fs::Disk_ptr disk, 
    net::Connection_ptr conn,
    net::TCP::buffer_t  buffer,
    size_t size)
{
  std::string reqstr((char*) buffer.get(), size);
  using namespace http;
  Request request(reqstr);
  
  printf("REQUEST URI: %s\n", request.get_uri().c_str());
  
  // get filesystem entry for request URI
  disk->fs().stat(request.get_uri(),
  [conn, disk] (fs::error_t err, auto& entry)
  {
    if (!entry.is_valid())
    {
      // return 404
      send_status(conn, disk, 404);
    }
    else if (entry.is_file())
    {
      send_file(conn, disk, entry);
    }
    else
    {
      send_directory(conn, disk, entry);
    }
  });
}

std::unique_ptr<net::Inet4<VirtioNet>> inet;
fs::Disk_ptr disk;

void Service::start() {
  
  // Assign a driver (VirtioNet) to a network interface (eth0)
  // @note: We could determine the appropirate driver dynamically, but then we'd
  // have to include all the drivers into the image, which  we want to avoid.
  hw::Nic<VirtioNet>& eth0 = hw::Dev::eth<0,VirtioNet>();
  
  // Bring up a network stack, attached to the nic
  // @note : No parameters after 'nic' means we'll use DHCP for IP config.
  static auto inet = std::make_unique<net::Inet4<VirtioNet> >(eth0);
  
  // Static IP configuration, until we (possibly) get DHCP
  // @note : Mostly to get a robust demo service that it works with and without DHCP
  inet->network_config(
      {{ 10,0,0,42 }},      // IP
			{{ 255,255,255,0 }},  // Netmask
			{{ 10,0,0,1 }},       // Gateway
			{{ 8,8,8,8 }} );      // DNS
  
  srand(OS::cycles_since_boot());

  // instantiate disk with FAT filesystem
  auto& device = hw::Dev::disk<1, VirtioBlk>();
  static auto disk = std::make_shared<fs::Disk> (device);
  assert(!disk->empty());
  
  disk->mount(
  [] (fs::error_t err) {
    if (err) {
      printf("Could not mount filesystem\n");
      panic("mount() failed");
    }
    
  });
  // Set up a TCP server on port 80
  printf("Setup server on port 80...\n");
  auto& server = inet->tcp().bind(80);
  server.onAccept(
  [] (auto) -> bool {
      return true;
  }).onConnect(
  [] (auto conn) {
    printf("Connected client\n");
    conn->read(1024, 
    [conn] (net::TCP::buffer_t buffer, size_t size) {
      printf("Read: %u\n", size);
      //client_read(disk, conn, buffer, size);
    });
  });
  printf("*** TEST SERVICE STARTED *** \n");
}
