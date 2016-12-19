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

#include <mana/middleware/cookie_parser.hpp>

namespace mana {
namespace middleware {

const std::regex Cookie_parser::cookie_pattern_ {"[^;]+"};

void Cookie_parser::process(mana::Request_ptr req, mana::Response_ptr, mana::Next next) {
  if(has_cookie(req)) {
    parse(read_cookies(req));
    auto jar_attr = std::make_shared<attribute::Cookie_jar>(req_cookies_);
    req->set_attribute(jar_attr);
  }

  return (*next)();
}

void Cookie_parser::parse(const std::string& cookie_data) {
  if(cookie_data.empty()) {
    throw http::CookieException{"Cannot parse empty cookie-string!"};
  }

  req_cookies_.clear(); //< Clear {req_cookies_} for new entries

  auto position = std::sregex_iterator(cookie_data.begin(), cookie_data.end(), cookie_pattern_);
  auto end      = std::sregex_iterator();

  while (position not_eq end) {
    auto cookie = (*position++).str();

    cookie.erase(std::remove(cookie.begin(), cookie.end(), ' '), cookie.end());

    auto pos = cookie.find('=');
    if (pos not_eq std::string::npos) {
      req_cookies_.insert(cookie.substr(0, pos), cookie.substr(pos + 1));
    } else {
      req_cookies_.insert(cookie);
    }
  }
}

}} //< mana::middleware
