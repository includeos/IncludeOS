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

#ifndef ROUTES_LANGUAGES_HPP
#define ROUTES_LANGUAGES_HPP
#include <mana/router.hpp>
#include <cookie/cookie_jar.hpp>

namespace acorn {
namespace routes {

class Languages : public mana::Router {
public:

  Languages()
  {
    on_get("/english", [](auto req, auto res) {
      Languages::lang_handler(req, res, "en-US");
    });

    on_get("/norwegian", [](auto req, auto res) {
      Languages::lang_handler(req, res, "nb-NO");
    });

    on_get("/clear", [](auto req, auto res) {
      Languages::clear(req, res);
    });
  }

private:

  static void lang_handler(mana::Request_ptr req, mana::Response_ptr res, const std::string& lang) {
    using namespace cookie;

    if (req->has_attribute<CookieJar>()) {
      auto req_cookies = req->get_attribute<CookieJar>();

      { // Print all the request-cookies
        const auto& all_cookies = req_cookies->get_cookies();
        for (const auto& c : all_cookies) {
          printf("Cookie: %s=%s\n", c.first.c_str(), c.second.c_str());
        }
      }

      const auto& value = req_cookies->cookie_value("lang");

      if (value == "") {
        printf("%s\n", "Cookie with name 'lang' not found! Creating it.");
        res->cookie(Cookie{"lang", lang});
      } else if (value not_eq lang) {
        printf("%s\n", "Cookie with name 'lang' found, but with wrong value. Updating cookie.");
        res->update_cookie<Cookie>("lang", lang);
      } else {
        printf("%s%s%s\n", "Wanted cookie already exists (name 'lang' and value '", lang.c_str(), "')!");
      }

    } else {
      printf("%s\n", "Request has no cookies! Creating cookie.");
      res->cookie(Cookie{"lang", lang});
    }

    res->send(true);
  }

  static void clear(mana::Request_ptr, mana::Response_ptr res) {
    using namespace cookie;
    printf("Clearing cookie!\n");
    res->clear_cookie<Cookie>("lang");
    res->send(true);
  }
};

} // < namespace routes
} // < namespace acorn

#endif
