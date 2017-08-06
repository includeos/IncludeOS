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

#ifndef ROUTES_SQUIRRELS_HPP
#define ROUTES_SQUIRRELS_HPP
#include <mana/router.hpp>
#include <models/squirrel.hpp>
#include <bucket/bucket.hpp>
#include <mana/attributes/json.hpp>

namespace acorn {
namespace routes {

class Squirrels : public mana::Router {

  using SquirrelBucket = bucket::Bucket<Squirrel>;

public:

  Squirrels(std::shared_ptr<SquirrelBucket> squirrels)
  {
    // GET /
    on_get("/",
    [squirrels] (auto, auto res)
    {
      //printf("[Squirrels@GET:/] Responding with content inside SquirrelBucket\n");
      using namespace rapidjson;
      StringBuffer sb;
      Writer<StringBuffer> writer(sb);
      squirrels->serialize(writer);
      res->send_json(sb.GetString());
    });

    // POST /
    on_post("/",
    [squirrels] (mana::Request_ptr req, auto res)
    {
      using namespace mana;
      auto json = req->get_attribute<attribute::Json_doc>();
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
          printf("[Squirrels@POST:/] Squirrel captured: %s\n", s.get_name().c_str());
          // setup the response
          // location to the newly created resource
          using namespace std;
          res->header().set_field(http::header::Location, req->uri().path().to_string()); // return back end loc i guess?
          // status code 201 Created
          res->source().set_status_code(http::Created);
          // send the created entity as response
          res->send_json(s.json());
        }
        catch(const Assert_error& e) {
          printf("[Squirrels@POST:/] Assert_error: %s\n", e.what());
          res->error({"Parsing Error", "Could not parse data."});
        }
        catch(const bucket::ConstraintException& e) {
          printf("[Squirrels@POST:/] ConstraintException: %s\n", e.what());
          res->error({"Constraint Exception", e.what()});
        }
        catch(const bucket::BucketException& e) {
          printf("[Squirrels@POST:/] BucketException: %s\n", e.what());
          res->error({"Bucket Exception", e.what()});
        }
      }
    });

  }
};

} // < namespace routes
} // < namespace acorn

#endif
