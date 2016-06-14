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

std::unique_ptr<server::Server> server_;
using namespace std;

#include <fs/disk.hpp>

////// DISK //////
// instantiate disk with filesystem
//#include <filesystem>
fs::Disk_ptr disk;

void recursive_fs_dump(vector<fs::Dirent> entries, int depth = 1) {
  auto& filesys = disk->fs();
  int indent = (depth * 3);
  for (auto entry : entries) {

    // Print directories
    if (entry.is_dir()) {
      // Normal dirs
      if (entry.name() != "."  and entry.name() != "..") {
        printf(" %*s-[ %s ]\n", indent, "+", entry.name().c_str());
        filesys.ls(entry, [depth](auto, auto entries) {
          recursive_fs_dump(*entries, depth + 1);
        });
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

#include "middleware/director.hpp"
#include "middleware/waitress.cpp"

#include "bucket.hpp"
#include "app/squirrel.hpp"

using namespace acorn;
using SquirrelBucket = bucket::Bucket<Squirrel>;
std::shared_ptr<SquirrelBucket> squirrels;

#include "middleware/parsley.hpp"

void Service::start() {

  auto& device = hw::Dev::disk<1, VirtioBlk>();
  disk = std::make_shared<fs::Disk> (device);

  uri::URI uri1("asdf");

  printf("<URI> Test URI: %s \n", uri1.to_string().c_str());

  // mount the main partition in the Master Boot Record
  disk->mount([](fs::error_t err) {

      if (err)  panic("Could not mount filesystem, retreating...\n");

      // setup "database"
      squirrels = std::make_shared<SquirrelBucket>();
      squirrels->add_index<std::string>("name", [](const Squirrel& s)->const auto& {
        return s.name;
      }, SquirrelBucket::UNIQUE);

      auto first_key = squirrels->spawn("Andreas"s, 28U, "Code Monkey"s).key;
      squirrels->spawn("Alf"s, 5U, "Script kiddie"s);

      Squirrel test_assert("Andreas", 0, "Tester");
      assert(squirrels->capture(test_assert) == 0);
      // no-go if throw
      assert(squirrels->look_for("name", "Andreas"s).key == first_key);

      server::Router routes;

      routes.on_get("/api/users/.*", [](auto, auto res) {
          res->add_header(http::header_fields::Entity::Content_Type,
                          "text/JSON; charset=utf-8"s)
            .add_body("{\"id\" : 1, \"name\" : \"alfred\"}"s);

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
          disk->fs().read_file("/index.html", [res] (fs::error_t err, fs::buffer_t buff, size_t len) {
              if(err) {
                res->set_status_code(http::Not_Found);
              } else {
                // fill Response with content from index.html
                printf("[@GET:/] Responding with index.html. \n");
                res->add_header(http::header_fields::Entity::Content_Type, "text/html; charset=utf-8"s)
                  .add_body(std::string{(const char*) buff.get(), len});
              }
              res->send();
            });

        }); // << fs().readFile

      routes.on_get("/api/squirrels", [](auto, auto res) {
        printf("[@GET:/api/squirrels] Responding with content inside SquirrelBucket\n");
        using namespace rapidjson;
        StringBuffer sb;
        Writer<StringBuffer> writer(sb);
        squirrels->serialize(writer);
        res->send_json(sb.GetString());
      });

      routes.on_post("/api/squirrels", [](server::Request_ptr req, auto res) {
        using namespace json;
        auto json = req->get_attribute<JsonDoc>();
        if(!json) {
          res->send_code(http::Bad_Request);
        }
        else {
          auto& doc = json->doc();
          try {
            // create an empty model
            acorn::Squirrel s;
            // deserialize it
            s.deserialize(doc);
            // add to bucket
            auto id = squirrels->capture(s);
            assert(id == s.key);
            printf("[@POST:/api/squirrels] Squirrel captured: %s\n", s.json().c_str());
            // setup the response
            // location to the newly created resource
            res->add_header(http::header_fields::Response::Location, "/api/squirrels/"s); // return back end loc i guess?
            // status code 201 Created
            res->set_status_code(http::Created);
            // send the created entity as response
            res->send_json(s.json());
          }
          catch(AssertException e) {
            res->send_code(http::Bad_Request);
            printf("[@POST:/api/squirrels] Exception: %s\n", e.what());
          }
        }

      });

      routes.on_get(".*", [](auto, auto res){
        printf("[@GET:*] Fallback route - try to serve index.html\n");
        disk->fs().stat("/public/index.html", [res](auto err, const auto& entry) {
          if(err) {
            res->send_code(http::Not_Found);
          } else {
            // Serve index.html
            printf("[@GET:*] (Fallback) Responding with index.html. \n");
            res->send_file({disk, entry});
          }
        });
      });
      // initialize server
      server_ = std::make_unique<server::Server>();
      server_->set_routes(routes).listen(80);

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
      auto opt = {"index.html", "index.htm"};
      //server::Middleware_ptr waitress = std::make_shared<Waitress>(disk, "", opt); // original
      server::Middleware_ptr waitress = std::make_shared<Waitress>(disk, "/public", opt); // WIP
      server_->use(waitress);

      // custom middleware to serve a webpage for a directory
      server::Middleware_ptr director = std::make_shared<Director>(disk, "/public/static");
      server_->use("/static", director);

      server::Middleware_ptr parsley = std::make_shared<Parsley>();
      server_->use(parsley);

      hw::PIT::instance().onRepeatedTimeout(15s, []{
        printf("%s\n", server_->ip_stack().tcp().status().c_str());
      });

    }); // < disk*/
}
