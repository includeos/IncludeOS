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

#include "cookie.hpp"
#include "middleware.hpp"

namespace middleware {

/**
 * @brief A way to parse cookies: Reading cookies that the browser is sending to the server
 * @details
 */
class CookieParser : public server::Middleware {
public:
  /**
   *
   */
  virtual void process(server::Request_ptr req, server::Response_ptr res, server::Next next) override;

  // Just name this method cookie?
  // Return bool or void?

  /**
   *
   */
  bool create_cookie(std::string& key, std::string& value);  // new Cookie(...) and add_header: Set-Cookie ?

  // Just name this method cookie?
  // Return bool or void?

  /**
   *
   */
  bool create_cookie(std::string& key, std::string& value, std::string& options);  // new Cookie(...) and add_header: Set-Cookie ?
// options: map eller enum

  // Return bool or void?

  /**
   *
   */
  bool clear_cookie(std::string& key); // remove Cookie from client

private:
  /**
   *
   */
  bool has_cookie(server::Request_ptr req) const noexcept;
}; //< class CookieParser

/**--v----------- Implementation Details -----------v--**/

inline void CookieParser::process(server::Request_ptr req, server::Response_ptr res, server::Next next) {








    if(not has_cookie(req)) {
      // No Cookie in header field: We want to create a cookie then??:
      // create cookie:
      // ...

      return (*next)();
    } else {
      // Found Cookie in header
      // Do something
    }
}

inline bool create_cookie(std::string& key, std::string& value) {

}

inline bool create_cookie(std::string& key, std::string& value, std::string& options) {

}

inline bool clear_cookie(std::string& key) {

}

inline bool CookieParser::has_cookie(server::Request_ptr req) const noexcept {
  return req->has_header(http::header_fields::Request::Cookie);
}

/**--^----------- Implementation Details -----------^--**/

} //< namespace middleware

#endif //< MIDDLEWARE_COOKIE_PARSER_HPP
