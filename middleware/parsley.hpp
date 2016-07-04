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

#ifndef MIDDLEWARE_PARSLEY_HPP
#define MIDDLEWARE_PARSLEY_HPP

#include "json.hpp"
#include "middleware.hpp"

namespace middleware {

/**
 * @brief A vegan way to parse JSON Content in a response
 * @details TBC..
 *
 */
class Parsley : public server::Middleware {
public:
  /**
   *
   */
  virtual void process(server::Request_ptr req, server::Response_ptr, server::Next next) override;

private:
  /**
   *
   */
  bool has_json(server::Request_ptr req) const;
}; //< class Parsley

/**--v----------- Implementation Details -----------v--**/

inline void Parsley::process(server::Request_ptr req, server::Response_ptr, server::Next next) {
  using namespace json;

  if(not has_json(req)) return (*next)();

  // Request doesn't have JSON attribute
  if(not req->has_attribute<JsonDoc>()) {
    // Create attribute
    auto json = std::make_shared<JsonDoc>();

    // Access the document and parse the body
    json->doc().Parse(req->get_body().c_str());
    printf("<Parsley> Parsed JSON data.\n");

    // Add the json attribute to the request
    req->set_attribute(json);
  }

  return (*next)();
}

inline bool Parsley::has_json(server::Request_ptr req) const {
  auto c_type = http::header_fields::Entity::Content_Type;
  if(not req->has_header(c_type)) return false;
  return (req->header_value(c_type).find("application/json") != std::string::npos);
}

/**--^----------- Implementation Details -----------^--**/

} //< namespace middleware

#endif //< MIDDLEWARE_PARSLEY_HPP
