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


template <typename PTR>
class BufferWrapper {

  using ptr_t = PTR;

  ptr_t data;
  size_t size;

public:

  BufferWrapper(ptr_t ptr, size_t sz) :
    data {ptr}, size{sz}
  {}

  const ptr_t begin() { return data; }
  const ptr_t end() { return data + size; }
};

#include "middleware/waitress.cpp"

#include <rapidjson/document.h>
#define RAPIDJSON_THROWPARSEEXCEPTION 1

std::shared_ptr<rapidjson::Document> squirrels_db;
static size_t DB_IDX = 0;


template <typename T>
std::string json_val(T val) {
  std::stringstream ss;
  ss << "\"" << val << "\"";
  return ss.str();
}

std::string create_squirrel(std::string name, size_t age) {
  const std::string ID = json_val("id");
  const std::string NAME = json_val("name");
  const std::string AGE = json_val("age");

  std::stringstream ss;
  ss << "{"
    << NAME << ":" << json_val(name) << ", "
    << AGE << ":" << json_val(age)
    << "}";

  return ss.str();
}

void seed_squirrels() {
  std::stringstream ss;

  ss << "{"
    << json_val(DB_IDX++) << ":" << create_squirrel("Johnnie", 42) << ", "
    << json_val(DB_IDX++) << ":" << create_squirrel("Felix", 13) << ", "
    << json_val(DB_IDX++) << ":" << create_squirrel("Eloise", 34)
    << "}";

  printf("JSON: %s\n", ss.str().c_str());
  auto& db = *squirrels_db;
  db.Parse(ss.str().c_str());
}

void setup_database() {
  squirrels_db = std::make_shared<rapidjson::Document>();
  DB_IDX = 1;

  seed_squirrels();
  auto& db = *squirrels_db;
  db["1"].GetObject()["name"].GetString();
}

void Service::start() {

  uri::URI uri1("asdf");

  printf("<URI> Test URI: %s \n", uri1.to_string().c_str());

  // mount the main partition in the Master Boot Record
  disk->mount([](fs::error_t err) {

      if (err)  panic("Could not mount filesystem, retreating...\n");

      server::Router routes;

      routes.on_get("/api/users/.*", [](auto req, auto res) {
          res->add_header(http::header_fields::Entity::Content_Type,
                          "text/JSON; charset=utf-8"s)
            .add_body("{\"id\" : 1, \"name\" : \"alfred\"}"s);

          res->send();
        });

      routes.on_get("/books/.*", [](auto req, auto res) {
          res->add_header(http::header_fields::Entity::Content_Type,
                          "text/HTML; charset=utf-8"s)
            .add_body("<html><body>"
                      "<h1>Books:</h1>"
                      "<ul>"
                      "<li> borkman.txt </li>"
                      "<li> fables.txt </li>"
                      "<li> poetics.txt </li>"
                      "</ul>"
                      "</body></html>"s
                      );

          res->send();
        });

      routes.on_get("/images/.*", [](auto req, auto res) {
          disk->fs().stat(req->uri().path(), [res](auto err, const auto& entry) {
              if(!err)
                res->send_file({disk, entry});
              else
                res->send_code(http::Not_Found);
        });

      });

      /* Route: GET / */
      routes.on_get(R"(index\.html?|\/|\?)", [](auto, auto res){
          disk->fs().readFile("/index.html", [res] (fs::error_t err, fs::buffer_t buff, size_t len) {
              if(err) {
                res->set_status_code(http::Not_Found);
              } else {
                // fill Response with content from index.html
                printf("<Route#GET:/> Responding with index.html. \n");
                res->add_header(http::header_fields::Entity::Content_Type, "text/html; charset=utf-8"s)
                  .add_body(std::string{(const char*) buff.get(), len});
              }
              res->send();
            });

        }); // << fs().readFile

      setup_database();
      routes.on_get("/api/squirrels", [](auto, auto res) {
      });
      // initialize server
      acorn = std::make_unique<server::Server>();
      acorn->set_routes(routes).listen(8081);

      /*
      // add a middleware as lambda
      acorn->use([](auto req, auto res, auto next){
        hw::PIT::on_timeout(0.050, [next]{
          printf("<MW:lambda> EleGiggle (50ms delay)\n");
          (*next)();
        });
      });
      */


      // custom middleware to serve static files
      server::Middleware_ptr waitress = std::make_shared<Waitress>(disk);
      acorn->use(waitress);


      auto vec = disk->fs().ls("/").entries;


      printf("------------------------------------ \n");
      printf(" Memdisk contents \n");
      printf("------------------------------------ \n");
      recursive_fs_dump(*vec);
      printf("------------------------------------ \n");

      hw::PIT::instance().onRepeatedTimeout(15s, []{
        printf("%s\n", acorn->ip_stack().tcp().status().c_str());
      });

    }); // < disk*/
}
