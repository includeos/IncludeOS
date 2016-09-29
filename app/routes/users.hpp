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

#ifndef ROUTES_USERS_HPP
#define ROUTES_USERS_HPP
#include <mana/router.hpp>
#include <models/user.hpp>
#include <bucket/bucket.hpp>
#include <json/json.hpp>

namespace acorn {
namespace routes {

class Users : public mana::Router {

  using UserBucket = bucket::Bucket<User>;

public:

  Users(std::shared_ptr<UserBucket> users)
  {
    // GET /
    on_get("/",
    [users] (auto, auto res)
    {
      printf("[Users@GET:/] Responding with content inside UserBucket\n");
      using namespace rapidjson;
      StringBuffer sb;
      Writer<StringBuffer> writer(sb);
      users->serialize(writer);
      res->send_json(sb.GetString());
    });

    // GET /:id(Digit)/:name/something/:something[Letters]
    on_get("/:id(\\d+)/:name/something/:something([a-z]+)",
    [users] (mana::Request_ptr req, auto res)
    {
      // Get parameters:
      // Alt.: std::string id = req->params().get("id");
      auto& params = req->params();
      std::string id = params.get("id");
      std::string name = params.get("name");
      std::string something = params.get("something");

      // std::string doesntexist = params.get("doesntexist");  // throws ParamException

      printf("id: %s\n", id.c_str());
      printf("name: %s\n", name.c_str());
      printf("something: %s\n", something.c_str());

      printf("[Users@GET:/:id(\\d+)/:name/something/:something([a-z]+)] Responding with content inside UserBucket\n");
      using namespace rapidjson;
      StringBuffer sb;
      Writer<StringBuffer> writer(sb);
      users->serialize(writer);
      res->send_json(sb.GetString());
    });


  }
};

} // < namespace routes
} // < namespace acorn

#endif
