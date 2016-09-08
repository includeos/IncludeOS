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

#include <profile>

#include <fs/disk.hpp>
#include <rtc>
#include <routes>
#include <dashboard>
#include <logger/logger.hpp>

using namespace std;
using namespace acorn;

using UserBucket     = bucket::Bucket<User>;
using SquirrelBucket = bucket::Bucket<Squirrel>;

std::shared_ptr<UserBucket>     users;
std::shared_ptr<SquirrelBucket> squirrels;

std::unique_ptr<server::Server> server_;
std::unique_ptr<dashboard::Dashboard> dashboard_;
std::unique_ptr<Logger> logger_;

////// DISK //////
// instantiate disk with filesystem
//#include <filesystem>
fs::Disk_ptr disk;

Statistics stats;
RTC::timestamp_t STARTED_AT;

#include <time.h>

// Get current date/time, format is YYYY-MM-DD.HH:mm:ss
const std::string currentDateTime() {
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    // Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
    // for more information about date/time format
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

    return buf;
}

void recursive_fs_dump(vector<fs::Dirent> entries, int depth = 1);

void Service::start(const std::string&) {

  /** SETUP LOGGER */
  char* buffer = (char*)malloc(1024*16);
  static gsl::span<char> spanerino{buffer, 1024*16};
  logger_ = std::make_unique<Logger>(spanerino);
  logger_->flush();
  logger_->log("LUL\n");

  printf("Going dark in 3 ... 2 ... 1 .\n");
  OS::set_rsprint([] (const char* data, size_t len) {
    OS::default_rsprint(data, len);
    logger_->log({data, len});
  });

  disk = fs::new_shared_memdisk();

  // mount the main partition in the Master Boot Record
  disk->mount([](fs::error_t err) {

      if (err)  panic("Could not mount filesystem, retreating...\n");

      /** IP STACK SETUP **/
      // Bring up IPv4 stack on network interface 0
      auto& stack = net::Inet4::ifconfig<0>(5.0,
      [](bool timeout) {
        printf("DHCP Resolution %s.\n", timeout?"failed":"succeeded");
      });
      // config
      stack.network_config({ 10,0,0,42 },     // IP
                           { 255,255,255,0 }, // Netmask
                           { 10,0,0,1 },      // Gateway
                           { 8,8,8,8 });      // DNS

      // only works with synchrone disks (memdisk)
      recursive_fs_dump(*disk->fs().ls("/").entries);

      /** STATS SETUP **/
      // TODO: Make use of Statman
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


      /** BUCKET SETUP */

      // create squirrel bucket
      squirrels = std::make_shared<SquirrelBucket>();
      // set member name to be unique
      squirrels->add_index<std::string>("name",
      [](const Squirrel& s)->const auto&
      {
        return s.get_name();
      }, SquirrelBucket::UNIQUE);

      // seed squirrels
      auto first_key = squirrels->spawn("Andreas"s, 28U, "Code Monkey"s).key;
      squirrels->spawn("Alf"s, 6U, "Script kiddie"s);

      // A test to see if constraint is working (squirrel).
      // TODO: Unit test bucket and remove this
      bool exception_thrown = false;
      try {
        Squirrel dupe_name("Andreas", 0, "Tester");
        squirrels->capture(dupe_name);
      } catch(bucket::ConstraintUnique) {
        exception_thrown = true;
      }
      assert(exception_thrown);

      // A test to see if field search is working
      assert(squirrels->look_for("name", "Andreas"s).key == first_key);

      // setup users bucket
      users = std::make_shared<UserBucket>();

      users->spawn();
      users->spawn();


      /** ROUTES SETUP **/

      server::Router router;

      // setup Squirrel routes
      router.use("/api/squirrels", routes::Squirrels{squirrels});
      // setup User routes
      router.use("/api/users", routes::Users{users});

      /*----------[START TEST OF] CookieJar and CookieParser----------*/
      auto lang_api_handler = [](server::Request_ptr req, auto res, const std::string& lang) {
        if (req->has_attribute<CookieJar>()) {
          auto req_cookies = req->get_attribute<CookieJar>();

          { // Print all the request-cookies
            const auto& all_cookies = req_cookies->get_cookies();
            for (const auto& c : all_cookies) {
              printf("Cookie: %s=%s\n", c.first.c_str(), c.second.c_str());
            }
          }

          const auto& value = req_cookies->cookie_value("lang");

          if (value == "") {
            printf("%s\n", "Cookie with name 'lang' not found! Creating it.");
            res->cookie("lang", lang);
          } else if (value not_eq lang) {
            printf("%s\n", "Cookie with name 'lang' found, but with wrong value. Updating cookie.");
            res->update_cookie("lang", "", "", lang);
          } else {
            printf("%s %s %s\n", "Wanted cookie already exists (name 'lang' and value '", lang.c_str() ,"')!");
            res->send(true);
          }

        } else {
          printf("%s\n", "Request has no cookies! Creating cookie.");
          res->cookie("lang", lang);
        }
      };

      router.on_get("/api/english", [&lang_api_handler](server::Request_ptr req, auto res) {
        lang_api_handler(req, res, "en-US");
      });

      router.on_get("/api/norwegian", [&lang_api_handler](server::Request_ptr req, auto res) {
        lang_api_handler(req, res, "nb-NO");
      });
      /*----------[END TEST OF] CookieJar and CookieParser----------*/

      // Temporary route for polling http stats
      // TODO: Refactor to use Statsman and maybe integrate with dashboard
      router.on_get("/api/stats", [](auto, auto res) {
        using namespace rapidjson;
        StringBuffer sb;
        Writer<StringBuffer> writer(sb);
        stats.set_active_clients(server_->active_clients())
             .set_memory_usage(OS::heap_usage())
             .serialize(writer);
        res->send_json(sb.GetString());
      });


      /** DASHBOARD SETUP **/
      dashboard_ = std::make_unique<dashboard::Dashboard>(8192);
      // Add singleton component
      dashboard_->add(dashboard::Memmap::instance());
      dashboard_->add(dashboard::StackSampler::instance());
      dashboard_->add(dashboard::Status::instance());
      // Construct component
      dashboard_->construct<dashboard::Statman>(Statman::get());
      dashboard_->construct<dashboard::TCP>(stack.tcp());
      dashboard_->construct<dashboard::CPUsage>(IRQ_manager::get(), 100ms, 2s);
      dashboard_->construct<dashboard::Logger>(*logger_, static_cast<size_t>(0));

      // Add Dashboard routes to "/api/dashboard"
      router.use("/api/dashboard", dashboard_->router());

      // Fallback route - serve index.html if route is not found
      router.on_get(".*", [](auto, auto res) {
        #ifdef VERBOSE_WEBSERVER
        printf("[@GET:*] Fallback route - try to serve index.html\n");
        #endif
        disk->fs().stat("/public/index.html", [res](auto err, const auto& entry) {
          if(err) {
            res->send_code(http::Not_Found);
          } else {
            // Serve index.html
            #ifdef VERBOSE_WEBSERVER
            printf("[@GET:*] (Fallback) Responding with index.html. \n");
            #endif
            res->send_file({disk, entry});
          }
        });
      });

      INFO("Router", "Registered routes:\n%s", router.to_string().c_str());


      /** SERVER SETUP **/

      // initialize server
      server_ = std::make_unique<server::Server>(stack);
      // set routes and start listening
      server_->set_routes(router).listen(80);


      /** MIDDLEWARE SETUP **/

      /* // example of how to inject a custom middleware closure
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

      // Print TCP information every 1 min
      Timers::periodic(30s, 1min, [](auto){
        printf("@onTimeout [%s]\n%s\n",
          currentDateTime().c_str(), server_->ip_stack().tcp().status().c_str());
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
