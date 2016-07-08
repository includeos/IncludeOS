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

//#include <ctime>
#include <time.h>

#include "middleware.hpp"
#include "cookiejar.hpp"
#include "cookie.hpp"

// using namespace cookie;

namespace middleware {

/**
 * @brief A way to parse cookies: Reading cookies that the browser is sending to the server
 * @details
 */
class CookieParser : public server::Middleware {

public:

  // TODO: Look at parsley.hpp (req->add_attribute<Cookie>)

  // TODO: Remove later
  CookieParser();

  CookieParser(CookieJar& jar);

  ~CookieParser();

  /**
   *
   */
  virtual void process(server::Request_ptr req, server::Response_ptr res, server::Next next) override;

  void create_cookie(const std::string& name, const std::string& value);

  /**
   *
   */
  bool create_cookie(server::Response_ptr res, const std::string& name, const std::string& value);

  /**
   *
   */
  bool create_cookie(server::Response_ptr res, const std::string& name, const std::string& value, const std::vector<std::string>& options);

  /**
   *
   */
  void parse_cookie(std::string& cs);

  /**
   *
   */
  bool clear_cookie(server::Response_ptr res);
  //bool clear_cookie(std::string& name);
    // remove Cookie from client

private:

  // CookieParser take a CookieJar as parameter (created in the service.cpp)
  CookieJar jar_;

  /**
   *
   */
  bool has_cookie(server::Request_ptr req) const noexcept;

  const std::string& get_cookie(server::Request_ptr req) const noexcept;

}; //< class CookieParser

/**--v----------- Implementation Details -----------v--**/

// TODO: Remove later
CookieParser::CookieParser() {

}

CookieParser::CookieParser(CookieJar& jar) : jar_{jar} {

}

CookieParser::~CookieParser() {

}

inline void CookieParser::process(server::Request_ptr req, server::Response_ptr res, server::Next next) {
  // using namespace cookie;

  // only set-cookie in response's header if the request has no cookie header field (for now)

  // parse creates the cookie object from string values

  // serialize creates the cookie string that is going into the header

  if(not has_cookie(req)) {
    // Testing:
    std::string name{"Cookie-test"};
    std::string value{"cookie-value"};
    // No Cookie in header field: We want to create a cookie then and use
    // the name and value fields that have already been set by the developer
    create_cookie(res, name, value);

    /* Want to change to (setting values through cookie_parser->create_cookie(name, value)):
    if(!name_.empty() && !value_.empty()) {
      cookie::Cookie c{name_, value_};
      std::string cs = c.serialize(name_, value_);

      printf("Created cookie: %s ", cs.c_str());

      res->add_header(http::header_fields::Response::Set_Cookie, cs);
    }*/

    return (*next)();
  } else {

    // Found Cookie in header - get the info

    std::string cs = get_cookie(req);
    parse_cookie(cs);


  }
}

void CookieParser::create_cookie(const std::string& name, const std::string& value) {

  // try catch
  cookie::Cookie c{name, value};

  // add to cookiejar if everything ok (no exception)

  c.serialize();

/*  name_ = name;
  value_ = value;*/

  // or: c{name, value};
  // Then call c.serialize() in process-method (fields already set in constructor to c)
  //
  // Do we need a CookieJar to place all the cookies in so that cookie_parser has a
  // cookie_jar with cookies...?
  // Or isn't this necessary (only need to set one cookie at a time..? No, the
  // developer probably want to create many cookies in service.cpp and therefore
  // wants many cookies to be set when the user enters a path/url..? And/or want
  // to set different cookies for different paths - want only one cookie_parser
  // per service.cpp) ?
}

inline bool CookieParser::create_cookie(server::Response_ptr res, const std::string& name, const std::string& value) {

  // try catch
  cookie::Cookie c{name, value};
  std::string cs = c.serialize();

  printf("Created cookie: %s ", cs.c_str());

  res->add_header(http::header_fields::Response::Set_Cookie, cs);

  // we also want to store the cookie in bucket - use CookieJar ?

  return true;
}

inline bool CookieParser::create_cookie(server::Response_ptr res, const std::string& name, const std::string& value, const std::vector<std::string>& options) {

  // try catch
  cookie::Cookie c{name, value, options};
  std::string cs = c.serialize();
  res->add_header(http::header_fields::Response::Set_Cookie, cs);

  // we also want to store the cookie in bucket - use CookieJar ?

  return true;
}

// return Cookie instead of void? The developer wants to see what the cookie from the browser
// contains, especially (only?) (name=) value
// If a cookie is called theme and has value red, the developer wants to do something
// If a cookie is called theme and has value black, the developer wants to do another thing
// Add to req (req->add_attribute<Cookie>)
inline void CookieParser::parse_cookie(std::string& cs) {
  // create cookie object from cookie data gotten from request header:
  // cookie::Cookie c{cs}; // calls parse-method - can throw exception - change ??

  // printf("Cookie looks like: %s", c.to_string().c_str());

  // find object in bucket/CookieJar and return the user (settings (key for now)) with this cookie ?
}

inline bool CookieParser::clear_cookie(server::Response_ptr res) {

  // find Cookie in CookieJar (?) and alter the expires field (in the past)

  /* try catch
  cookie::Cookie c{"name", "value"};

  //time_t time = time(NULL);  // now - 1
  time_t t = time(0);
  struct tm* pmt = gmtime(&t);

  if(pmt == NULL)
    return false;
  */

  //strftime(%Y-%m-%dT%H:%M:%S%Z);

  //strftime(%a );

  //std::string time_string{std::to_string(pmt->tm_hour % 24) + ":" + std::to_string(pmt->tm_min) + ":"};

  // if < 10 (timer, min, sek): legge til 0 foran

  // Format on Expires value: Sun, 01-Jan-2016 22:00:02 GMT

  // Check out to_string in src/hw/cmos.cpp (IncludeOS)

  //std::vector<std::string> v{"Expires", time_string};
  /*c.set_expires(time);
  std::string cs = c.serialize();
  res->add_header(http::header_fields::Response::Set_Cookie, cs);*/

  // remove from CookieJar (if successful clear...?)

  // php: setcookie("yourcookie","yourvalue",time()-1);
  // it expired so it's deleted
  // RFC 6265 5.3: A cookie is "expired" if the cookie has an expiry date in the past.

  return true;
}

inline bool CookieParser::has_cookie(server::Request_ptr req) const noexcept {
  return req->has_header(http::header_fields::Request::Cookie);
}

inline const std::string& CookieParser::get_cookie(server::Request_ptr req) const noexcept {
  return req->header_value(http::header_fields::Request::Cookie);
}

/**--^----------- Implementation Details -----------^--**/

} //< namespace middleware

#endif //< MIDDLEWARE_COOKIE_PARSER_HPP
