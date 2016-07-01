#ifndef MIDDLEWARE_COOKIE_PARSER_HPP
#define MIDDLEWARE_COOKIE_PARSER_HPP

#include "../cookie/cookie.hpp"
#include "middleware.hpp"

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

#endif
