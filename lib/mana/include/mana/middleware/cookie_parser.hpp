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

#ifndef MANA_MIDDLEWARE_COOKIE_PARSER_HPP
#define MANA_MIDDLEWARE_COOKIE_PARSER_HPP

#include <net/http/cookie.hpp>
#include <mana/attributes/cookie_jar.hpp>
#include <mana/middleware.hpp>

namespace mana {
namespace middleware {

/**
 * @brief A way to parse cookies that the browser is sending to the server
 */
class Cookie_parser : public Middleware {
public:

  Callback handler() override {
    return {this, &Cookie_parser::process};
  }

  void process(Request_ptr req, Response_ptr res, Next next);

private:
  attribute::Cookie_jar req_cookies_;

  static const std::regex cookie_pattern_;

  bool has_cookie(mana::Request_ptr req) const noexcept;

  std::string read_cookies(mana::Request_ptr req) const noexcept;

  void parse(const std::string& cookie_data);

}; // < class CookieParser

/**--v----------- Implementation Details -----------v--**/

inline bool Cookie_parser::has_cookie(mana::Request_ptr req) const noexcept {
  return req->header().has_field(http::header::Cookie);
}

inline std::string Cookie_parser::read_cookies(mana::Request_ptr req) const noexcept {
  return std::string(req->header().value(http::header::Cookie));
}

/**--^----------- Implementation Details -----------^--**/

}} //< namespace mana::middleware

#endif //< MIDDLEWARE_COOKIE_PARSER_HPP
