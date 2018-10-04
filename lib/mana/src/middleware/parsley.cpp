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


#include <mana/middleware/parsley.hpp>
#include <mana/attributes/json.hpp>

namespace mana {
namespace middleware {

void Parsley::process(mana::Request_ptr req, mana::Response_ptr, mana::Next next) {

  // Request doesn't have JSON attribute
  if(has_json(*req) and not req->has_attribute<attribute::Json_doc>())
  {
    // Create attribute
    auto json = std::make_shared<attribute::Json_doc>();

    // Access the document and parse the body
    bool err = json->doc().Parse(req->source().body().data()).HasParseError();
    #ifdef VERBOSE_WEBSERVER
    printf("<Parsley> Parsed JSON data.\n");
    #endif

    if(not err)
      req->set_attribute(std::move(json));
    else
      printf("<Parsley> Parsing error\n");
  }

  return (*next)();
}

bool Parsley::has_json(const mana::Request& req) const {
  auto c_type = http::header::Content_Type;
  if(not req.header().has_field(c_type)) return false;
  return (req.header().value(c_type).find("application/json") != std::string::npos);
}

}} // < namespace mana::middleware
