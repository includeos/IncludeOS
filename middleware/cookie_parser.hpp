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

#include "middleware.hpp"
#include "cookie_jar.hpp"
#include "cookie.hpp"

using namespace cookie;

namespace middleware {

/**
 * @brief A way to parse cookies that the browser is sending to the server
 */
class CookieParser : public server::Middleware {

public:
  virtual void process(server::Request_ptr req, server::Response_ptr res, server::Next next) override;

private:
  CookieJar req_cookies_;

  static const std::regex pattern;

  bool has_cookie(server::Request_ptr req) const noexcept;

  const std::string& read_cookies(server::Request_ptr req) const noexcept;

  void parse(const std::string& cookie_data);

}; // < class CookieParser

/**--v----------- Implementation Details -----------v--**/

inline void CookieParser::process(server::Request_ptr req, server::Response_ptr, server::Next next) {
  if(has_cookie(req)) {
    // Get the cookies that already exists (sent in the request):
    std::string cookies_string = read_cookies(req);

    // Parse the string to the CookieJar req_cookies_
    parse(cookies_string);

    auto jar_attr = std::make_shared<CookieJar>(req_cookies_);
    req->set_attribute(jar_attr);
  }

  return (*next)();
}

const std::regex CookieParser::pattern{"[^;]+"};

inline bool CookieParser::has_cookie(server::Request_ptr req) const noexcept {
  return req->has_header(http::header_fields::Request::Cookie);
}

inline const std::string& CookieParser::read_cookies(server::Request_ptr req) const noexcept {
  return req->header_value(http::header_fields::Request::Cookie);
}

void CookieParser::parse(const std::string& cookie_data) {
  // From the client we only get name=value; for each cookie

  if(cookie_data.empty())
    throw CookieException{"Cannot parse empty cookie-string!"};

  req_cookies_.clear(); // Clear the CookieJar

  auto position = std::sregex_iterator(cookie_data.begin(), cookie_data.end(), pattern);
  auto end = std::sregex_iterator();

  for (std::sregex_iterator i = position; i != end; ++i) {
    std::smatch pair = *i;
    std::string pair_str = pair.str();

    // Remove all empty spaces:
    pair_str.erase(std::remove(pair_str.begin(), pair_str.end(), ' '), pair_str.end());

    size_t pos = pair_str.find("=");
    std::string name = pair_str.substr(0, pos);
    std::string value = pair_str.substr(pos + 1);

    req_cookies_.insert(name, value);
  }
}

/**--^----------- Implementation Details -----------^--**/

} // < namespace middleware

#endif //< MIDDLEWARE_COOKIE_PARSER_HPP
