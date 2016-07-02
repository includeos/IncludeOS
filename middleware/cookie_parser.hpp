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

#ifndef MIDDLEWARE_COOKIE_PARSER_HPP
#define MIDDLEWARE_COOKIE_PARSER_HPP

#include "../cookie/cookie.hpp"
#include "middleware.hpp"

namespace server {

/**
 * @brief A way to parse cookies: Reading cookies that the browser is sending to the server
 * @details
 *
 */

// hpp-declarations first and implement methods further down? Or in own cpp-file?

class CookieParser : public server::Middleware {

public:
  virtual void process(server::Request_ptr req, server::Response_ptr res, server::Next next) override {

    using namespace cookie;

    if(!has_cookie(req)) {
      //No Cookie in header field: We want to create a cookie then??:
      //
      //create cookie:

      (*next)();
      return;
    }

    // Found Cookie in header



  }

  // Just name this method cookie?
  // Return bool or void?
  bool create_cookie(std::string& key, std::string& value);  // new Cookie(...) and add_header: Set-Cookie ?

  // Just name this method cookie?
  // Return bool or void?
  bool create_cookie(std::string& key, std::string& value, std::string& options);  // new Cookie(...) and add_header: Set-Cookie ?
// options: map eller enum

  // Return bool or void?
  bool clear_cookie(std::string& key); // remove Cookie from client

private:
  bool has_cookie(server::Request_ptr req) const {
    auto c_type = http::header_fields::Request::Cookie;

    return req->has_header(c_type);
  }

};

} //< namespace server

#endif
