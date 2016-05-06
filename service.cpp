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
#include <sstream>
//#include <http>
#include "server/server.hpp"

std::unique_ptr<server::Server> acorn;

#include <memdisk>
#include <fs/fat.hpp> // FAT32 filesystem
using namespace fs;
using namespace std;

////// DISK //////
// instantiate disk with filesystem
auto disk = fs::new_shared_memdisk();

void recursive_fs_dump(vector<fs::Dirent> entries, int depth = 1) {
  auto& filesys = disk->fs();
  int indent = (depth * 3);
  for (auto entry : entries) {

    // Print directories
    if (entry.is_dir()) {
      // Normal dirs
      if (entry.name() != "."  and entry.name() != "..") {
        printf(" %*s-[ %s ]\n", indent, "+", entry.name().c_str());
        recursive_fs_dump(*filesys.ls(entry).entries, depth + 1 );
      } else {
        printf(" %*s  %s \n", indent, "+", entry.name().c_str());
      }

    }else {
      // Print files / symlinks etc.
      //printf(" %*s  \n", indent, "|");
      printf(" %*s-> %s \n", indent, "+", entry.name().c_str());
    }
  }
  printf(" %*s \n", indent, " ");
  //printf(" %*s \n", indent, "o");

}

void Service::start() {
  /*hw::Nic<VirtioNet>& eth0 = hw::Dev::eth<0,VirtioNet>();
  inet = std::make_unique<net::Inet4<VirtioNet> >(eth0);

  // Static IP configuration, until we (possibly) get DHCP
  // @note : Mostly to get a robust demo service that it works with and without DHCP
  inet->network_config( { 10,0,0,42 },      // IP
                        { 255,255,255,0 },  // Netmask
                        { 10,0,0,1 },       // Gateway
                        { 8,8,8,8 } );      // DNS
*/

  // mount the main partition in the Master Boot Record
  disk->mount([](fs::error_t err) {

      if (err)  panic("Could not mount filesystem\n");

      server::Router routes;

      /* Route: /images/prettypicture.jpg */
      routes.on_get("/images/prettypicture.jpg"s,
        [](const auto&, auto res)
      {

        disk->fs().stat("/images/prettypicture.jpg",
          [res](auto err, const auto& entry)
        {
          if(!err)
            res->send_file({disk, entry});
        });

      });

      /* Route: /images/prettypicture.jpg */
      routes.on_get("/"s, [](const auto&, auto res){
          disk->fs().readFile("/index.html", [&res] (fs::error_t err, fs::buffer_t buff, size_t len) {
              if(err) {
                res->set_status_code(http::Not_Found);
              } else {
                // fill Response with content from index.html
                printf("<Server> Responding with index.html. \n");
                res->add_header(http::header_fields::Response::Server, "IncludeOS/Acorn")
                  .add_header(http::header_fields::Entity::Content_Type, "text/html; charset=utf-8"s)
                  .add_header(http::header_fields::Response::Connection, "close"s)
                  .add_body(std::string{(const char*) buff.get(), len});
              }
            });

        }); // << fs().readFile

      // initialize server
      acorn = std::make_unique<server::Server>();
      acorn->set_routes(routes).listen(8081);

      auto vec = disk->fs().ls("/").entries;


      printf("------------------------------------ \n");
      printf(" Memdisk contents \n");
      printf("------------------------------------ \n");
      recursive_fs_dump(*vec);
      printf("------------------------------------ \n");

      hw::PIT::instance().onRepeatedTimeout(5s, []{
        printf("%s\n", acorn->ip_stack().tcp().status().c_str());
      });

    }); // < disk*/
}
