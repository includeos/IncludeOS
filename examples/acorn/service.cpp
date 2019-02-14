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
#include <acorn>

using namespace std;
using namespace acorn;

using UserBucket     = bucket::Bucket<User>;
using SquirrelBucket = bucket::Bucket<Squirrel>;

static std::shared_ptr<UserBucket>     users;
static std::shared_ptr<SquirrelBucket> squirrels;

static std::unique_ptr<mana::Server> server_;
static std::unique_ptr<mana::dashboard::Dashboard> dashboard_;
static std::unique_ptr<Logger> logger_;
// static content from memdisk
static fs::Disk_ptr disk;

#include <isotime>
#include <net/interfaces>

static void start_acorn(net::Inet& inet)
{
  /** SETUP LOGGER */
  const int LOGBUFFER_LEN = 1024*16;
  static gsl::span<char> spanerino{new char[LOGBUFFER_LEN], LOGBUFFER_LEN};
  logger_ = std::make_unique<Logger>(spanerino);
  logger_->flush();
  logger_->log("LUL\n");

  os::add_stdout(
  [] (const char* data, size_t len) {
    // append timestamp
    auto entry = "[" + isotime::now() + "]" + std::string{data, len};
    logger_->log(entry);
  });

  disk = fs::shared_memdisk();

  // init the first legit partition/filesystem
  disk->init_fs(
  [&inet] (fs::error_t err, auto& fs)
  {
      if (err) os::panic("Could not mount filesystem...\n");

      // only works with synchronous disks (memdisk)
      list_static_content(fs);

      /** BUCKET SETUP */

      // create squirrel bucket
      squirrels = std::make_shared<SquirrelBucket>(10);
      // set member name to be unique
      squirrels->add_index<std::string>("name",
      [](const Squirrel& s)->const auto&
      {
        return s.get_name();
      }, SquirrelBucket::UNIQUE);

      // seed squirrels
      squirrels->spawn("Alfred"s,  1000U, "Wizard"s);
      squirrels->spawn("Alf"s,     6U,    "Script Kiddie"s);
      squirrels->spawn("Andreas"s, 28U,   "Code Monkey"s);
      squirrels->spawn("AnnikaH"s, 20U,   "Fairy"s);
      squirrels->spawn("Ingve"s,   24U,   "Integration Master"s);
      squirrels->spawn("Martin"s,  16U,   "Build Master"s);
      squirrels->spawn("Rico"s,    28U,   "Mad Scientist"s);

      // setup users bucket
      users = std::make_shared<UserBucket>();
      users->spawn();
      users->spawn();

      /** ROUTES SETUP **/
      using namespace mana;
      Router router;

      // setup Squirrel routes
      router.use("/api/squirrels", routes::Squirrels{squirrels});
      // setup User routes
      router.use("/api/users", routes::Users{users});
      // setup Language routes
      router.use("/api/languages", routes::Languages{});


      /** DASHBOARD SETUP **/
      dashboard_ = std::make_unique<dashboard::Dashboard>(8192);
      // Add singleton component
      dashboard_->add(dashboard::Memmap::instance());
      dashboard_->add(dashboard::StackSampler::instance());
      dashboard_->add(dashboard::Status::instance());
      // Construct component
      dashboard_->construct<dashboard::Statman>(Statman::get());
      dashboard_->construct<dashboard::TCP>(inet.tcp());
      dashboard_->construct<dashboard::CPUsage>();
      dashboard_->construct<dashboard::Logger>(*logger_, static_cast<size_t>(50));

      // Add Dashboard routes to "/api/dashboard"
      router.use("/api/dashboard", dashboard_->router());

      // Fallback route for angular application - serve index.html if route is not found
      router.on_get("/app/.*",
      [&fs](auto, auto res) {
        #ifdef VERBOSE_WEBSERVER
        printf("[@GET:/app/*] Fallback route - try to serve index.html\n");
        #endif
        fs.cstat("/public/app/index.html", [res](auto err, const auto& entry) {
          if(err) {
            res->send_code(http::Not_Found);
          } else {
            // Serve index.html
            #ifdef VERBOSE_WEBSERVER
            printf("[@GET:/app/*] (Fallback) Responding with index.html. \n");
            #endif
            res->send_file({disk, entry});
          }
        });
      });
      INFO("Router", "Registered routes:\n%s", router.to_string().c_str());


      /** SERVER SETUP **/
      server_ = std::make_unique<Server>(inet.tcp());
      // set routes and start listening
      server_->set_routes(router).listen(80);


      /** MIDDLEWARE SETUP **/
      // custom middleware to serve static files
      auto opt = {"index.html"};
      Middleware_ptr butler = std::make_shared<middleware::Butler>(disk, "/public", opt);
      server_->use(butler);

      // custom middleware to serve a webpage for a directory
      Middleware_ptr director = std::make_shared<middleware::Director>(disk, "/public/static");
      server_->use("/static", director);

      Middleware_ptr parsley = std::make_shared<middleware::Parsley>();
      server_->use(parsley);

      Middleware_ptr cookie_parser = std::make_shared<middleware::Cookie_parser>();
      server_->use(cookie_parser);

    }); // < disk

}

void Service::start()
{
  auto& inet = net::Interfaces::get(0);
  if (not inet.is_configured())
  {
    inet.on_config(start_acorn);
  }
  else {
    start_acorn(inet);
  }
}
