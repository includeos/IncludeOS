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
#include "path_to_regexp.hpp"

using namespace route;

namespace middleware {

class RouteParser : public server::Middleware {

public:
  virtual void process(server::Request_ptr req, server::Response_ptr res, server::Next next) override;

private:

  PathToRegexp path_to_regexp;
  // or
  std::vector<Token> keys;  // What the developer wants access to in service.cpp
  // or


};  // < class RouteParser

/**--v----------- Implementation Details -----------v--**/

inline void RouteParser::process(server::Request_ptr req, server::Response_ptr res, server::Next next) {

  // From CookieParser:
  /*if(has_cookie(req)) {
    // Get the cookies that already exists (sent in the request):
    std::string cookies_string = read_cookies(req);

    // Parse the string to the CookieJar req_cookies_
    parse(cookies_string);

    auto jar_attr = std::make_shared<CookieJar>(req_cookies_);
    req->set_attribute(jar_attr);
  }*/



  return (*next)();
}

/**--^----------- Implementation Details -----------^--**/

};  // < namespace middleware

#endif  // < MIDDLEWARE_ROUTE_PARSER_HPP
