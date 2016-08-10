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

#include <sstream>

#include <os>
#include <acorn>
#include <memdisk>

#include <fs/disk.hpp>
#include <hw/cmos.hpp>

using namespace std;
using namespace acorn;

using UserBucket     = bucket::Bucket<User>;
using SquirrelBucket = bucket::Bucket<Squirrel>;

std::shared_ptr<UserBucket>     users;
std::shared_ptr<SquirrelBucket> squirrels;

std::unique_ptr<server::Server> server_;

////// DISK //////
// instantiate disk with filesystem
//#include <filesystem>
fs::Disk_ptr disk;

Statistics stats;
cmos::Time STARTED_AT;

void recursive_fs_dump(vector<fs::Dirent> entries, int depth = 1);

void Service::start() {

  // Test {URI} component
  uri::URI project_uri {"https://github.com/hioa-cs/IncludeOS"};
  printf("<URI> Test URI: %s \n", project_uri.path().c_str());

  disk = fs::new_shared_memdisk();

  // mount the main partition in the Master Boot Record
  disk->mount([](fs::error_t err) {

      if (err)  panic("Could not mount filesystem, retreating...\n");

      server::Connection::on_connection([](){
        stats.bump_connection_count();
      });

      server::Response::on_sent([](size_t n) {
        stats.bump_data_sent(n)
             .bump_response_sent();
      });

      server::Request::on_recv([](size_t n) {
        stats.bump_data_received(n)
          .bump_request_received();
      });

      // setup "database" squirrels

      squirrels = std::make_shared<SquirrelBucket>();
      squirrels->add_index<std::string>("name", [](const Squirrel& s)->const auto& {
        return s.get_name();
      }, SquirrelBucket::UNIQUE);

      auto first_key = squirrels->spawn("Andreas"s, 28U, "Code Monkey"s).key;
      squirrels->spawn("Alf"s, 5U, "Script kiddie"s);

      // A test to see if constraint is working (squirrel).
      bool exception_thrown = false;
      try {
        Squirrel dupe_name("Andreas", 0, "Tester");
        squirrels->capture(dupe_name);
      } catch(bucket::ConstraintUnique) {
        exception_thrown = true;
      }
      //assert(exception_thrown);

      // no-go if throw
      assert(squirrels->look_for("name", "Andreas"s).key == first_key);

      // setup "database" users

      users = std::make_shared<UserBucket>();
      /*users->add_index<size_t>("key", [](const User& u)->const auto& {
        return u.key;
      }, UserBucket::UNIQUE);*/

      //auto first_user_key = users->spawn().key;
      users->spawn();
      users->spawn();

      // A test to see if constraint is working (user).
      bool e_thrown = false;
      try {
        User dupe_id;
        dupe_id.key = 1;
        users->capture(dupe_id);
      } catch(bucket::ConstraintUnique) {
        e_thrown = true;
      }
      //assert(e_thrown);

      // no-go if throw
      //assert(users->look_for("key", 1).key == first_user_key);

      server::Router routes;

  // TODO TESTING CookieJar and CookieParser FROM HERE:

      routes.on_get("/api/english", [](server::Request_ptr req, auto res) {
        if(req->has_attribute<CookieJar>()) {
          auto req_cookies = req->get_attribute<CookieJar>();

          // Print all the request-cookies
          std::map<std::string, std::string> all_cookies = req_cookies->get_cookies();
          for(const auto& c : all_cookies)
            printf("Cookie: %s=%s\n", c.first.c_str(), c.second.c_str());

          std::string value = req_cookies->find("lang");

          if(value == "") {
            printf("Cookie with name 'lang' not found! Creating it.\n");
            res->cookie("lang", "en-US");
          } else if(value not_eq "en-US") {
            printf("Cookie with name 'lang' found, but with wrong value. Updating cookie.\n");
            res->update_cookie("lang", "", "", "en-US");
          } else {  // Cookie 'lang' exists and has wanted value ('en-US')
            printf("Wanted cookie already exists (name 'lang' and value 'en-US')!\n");
            res->send(true);
          }

        } else {
          printf("Request has no cookies! Creating cookie.\n");
          // Want to create lang-cookie then:
          res->cookie("lang", "en-US");
        }
      });

      routes.on_get("/api/norwegian", [](server::Request_ptr req, auto res) {
        if(req->has_attribute<CookieJar>()) {
          auto req_cookies = req->get_attribute<CookieJar>();

          // Print all the request-cookies
          std::map<std::string, std::string> all_cookies = req_cookies->get_cookies();
          for(const auto& c : all_cookies)
            printf("Cookie: %s=%s\n", c.first.c_str(), c.second.c_str());

          std::string value = req_cookies->find("lang");

          if(value == "") {
            printf("Cookie with name 'lang' not found! Creating it.\n");
            res->cookie("lang", "nb-NO");
          } else if(value not_eq "nb-NO") {
            printf("Cookie with name 'lang' found, but with wrong value. Updating cookie.\n");
            res->update_cookie("lang", "", "", "nb-NO");
          } else {  // Cookie 'lang' exists and has wanted value ('nb-NO')
            printf("Wanted cookie already exists (name 'lang' and value 'nb-NO')!\n");
            res->send(true);
          }

        } else {
          printf("Request has no cookies! Creating cookie.\n");
          // Want to create lang-cookie then:
          res->cookie("lang", "nb-NO");
        }
      });

  // UNTIL HERE

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
          res->error({http::Internal_Server_Error, "Server Error", "Server needs to be sprinkled with Parsley"});
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
            printf("[@POST:/api/squirrels] AssertException: %s\n", e.what());
            res->error({"Parsing Error", "Could not parse data."});
          }
          catch(bucket::ConstraintException e) {
            printf("[@POST:/api/squirrels] ConstraintException: %s\n", e.what());
            res->error({"Constraint Exception", e.what()});
          }
          catch(bucket::BucketException e) {
            printf("[@POST:/api/squirrels] BucketException: %s\n", e.what());
            res->error({"Bucket Exception", e.what()});
          }
        }
      });

      routes.on_get("/api/users", [](auto, auto res) {
        printf("[@GET:/api/users] Responding with content inside UserBucket\n");
        using namespace rapidjson;
        StringBuffer sb;
        Writer<StringBuffer> writer(sb);
        users->serialize(writer);
        res->send_json(sb.GetString());
      });

      routes.on_get("/api/stats", [](auto, auto res) {
        using namespace rapidjson;
        StringBuffer sb;
        Writer<StringBuffer> writer(sb);
        stats.set_active_clients(server_->active_clients())
             .set_memory_usage(OS::heap_usage())
             .serialize(writer);
        res->send_json(sb.GetString());
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

      STARTED_AT = cmos::now();

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
      server::Middleware_ptr waitress = std::make_shared<middleware::Waitress>(disk, "/public", opt); // WIP
      server_->use(waitress);

      // custom middleware to serve a webpage for a directory
      server::Middleware_ptr director = std::make_shared<middleware::Director>(disk, "/public/static");
      server_->use("/static", director);

      server::Middleware_ptr parsley = std::make_shared<middleware::Parsley>();
      server_->use(parsley);

      server::Middleware_ptr cookie_parser = std::make_shared<middleware::CookieParser>();
      server_->use(cookie_parser);

      hw::PIT::instance().on_repeated_timeout(1min, []{
        printf("@onTimeout [%s]\n%s\n",
          cmos::now().to_string().c_str(), server_->ip_stack().tcp().status().c_str());
      });

    }); // < disk
}

void recursive_fs_dump(vector<fs::Dirent> entries, int depth) {
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
