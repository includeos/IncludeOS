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

#ifndef MIDDLEWARE_ROUTE_PARSER_HPP
#define MIDDLEWARE_ROUTE_PARSER_HPP

#include "middleware.hpp"
#include "../route/path_to_regexp.hpp"

using namespace route;

namespace middleware {

class RouteParser : public server::Middleware {

public:
  virtual void process(server::Request_ptr req, server::Response_ptr res, server::Next next) override;

};  // < class RouteParser

/**--v----------- Implementation Details -----------v--**/

inline void RouteParser::process(server::Request_ptr req, server::Response_ptr res, server::Next next) {
  std::string path = req->uri().path();

  if(not path.empty()) {

    printf("Path: %s\n", path.c_str());

    // Params is now set in router.hpp (match-method) - how to get access to them here??
    // Or can we set the Params-attribute onto the request in the match-method?

    Params params;

    // ...

    auto params_attr = std::make_shared<Params>(params);
    req->set_attribute(params_attr);
  }
  else {
    printf("Path is empty!\n");
  }

  return (*next)();
}

/**--^----------- Implementation Details -----------^--**/

};  // < namespace middleware

#endif  // < MIDDLEWARE_ROUTE_PARSER_HPP
