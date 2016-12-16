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

#ifndef HTTP_COOKIE_HPP
#define HTTP_COOKIE_HPP

#include <regex>
#include <string>
#include <vector>
#include <cctype>
#include <utility>
#include <ostream>
#include <sstream>
#include <algorithm>
#include <functional>
#include <exception>

namespace http {

class CookieException : public std::exception {
  std::string msg;
  CookieException(){}

public:
  CookieException(const std::string& s) throw() : msg{s} {}
  const char* what() const throw() { return msg.c_str(); }
};  // < class CookieException

class Cookie {

public:
  explicit Cookie(const std::string& name, const std::string& value);

  explicit Cookie(const std::string& name, const std::string& value, const std::vector<std::string>& options);

  Cookie(const Cookie&) = default;

  Cookie(Cookie&&) noexcept = default;

  Cookie& operator = (const Cookie&) = default;

  Cookie& operator = (Cookie&&) = default;

  ~Cookie() = default;

  inline const std::string& get_name() const noexcept { return name_; }

  inline const std::string& get_value() const noexcept { return value_; }

  void set_value(const std::string& value);

  /**
   * @brief      [RFC 6265] The Expires attribute indicates the maximum lifetime of the cookie,
   *             represented as the date and time at which the cookie expires. The user agent
   *             is not required to retain the cookie until the specified date has passed. In fact,
   *             user agents often evict cookies due to memory pressure or privacy concerns.
   *
   * @return     expires_
   */
  inline const std::string& get_expires() const noexcept { return expires_; }

  /**
   * @brief      Sets the expires_ attribute.
   *
   * @param[in]  expires The expiry date for the cookie (if set in the past
   *             the cookie will be deleted/cleared from the browser)
   */
  void set_expires(const std::string& expires);

  /**
   * @brief      [RFC 6265] The Max-Age attribute indicates the maximum lifetime of the cookie,
   *             represented as the number of seconds until the cookie expires. The user agent
   *             is not required to retain the cookie for the specified duration. In fact,
   *             user agents often evict cookies due to memory pressure or privacy concerns.
   *
   *             NOTE: Some existing user agents do not support the Max-Age attribute. User agents
   *             that do not support the Max-Age attribute ignore the attribute.
   *
   *             If a cookie has both the Max-Age and the Expires attribute, the Max-Age attribute
   *             has precedence and controls the expiration date of the cookie. If a cookie has neither
   *             the Max-Age nor the Expires attribute, the user agent will retain the cookie until
   *             "the current session is over" (as defined by the user agent).
   *
   * @return     max_age_
   */
  inline int get_max_age() const noexcept { return max_age_; }

  void set_max_age(int max_age);

  /**
   * @brief      [RFC 6265] The Domain attribute specifies those hosts to which the cookie will be sent.
   *             For example, if the value of the Domain attribute is "example.com", the user agent
   *             will include the cookie in the Cookie header when making HTTP requests to example.com,
   *             www.example.com, and www.corp.example.com.
   *
   *             WARNING: Some existing user agents treat an absent Domain attribute as if the Domain
   *             attribute were present and contained the current host name. For example, if example.com
   *             returns a Set-Cookie header without a Domain attribute, these user agents will erroneously
   *             send the cookie to www.example.com as well.
   *
   *             The user agent will reject cookies unless the Domain attribute specifies a scope for the
   *             cookie that would include the origin server. For example, the user agent will accept a
   *             cookie with a Domain attribute of "example.com" or of "foo.example.com" from foo.example.com,
   *             but the user agent will not accept a cookie with a Domain attribute of "bar.example.com"
   *             or of "baz.foo.example.com".
   *
   *             NOTE: For security reasons, many user agents are configured to reject Domain attributes that
   *             correspond to "public suffixes". For example, some user agents will reject Domain attributes
   *             of "com" or "co.uk".
   *
   * @return     domain_
   */
  inline const std::string& get_domain() const noexcept { return domain_; }

  void set_domain(const std::string& domain);

  /**
   * @brief      [RFC 6265] The scope of each cookie is limited to a set of paths, controlled by the
   *             Path attribute. If the server omits the Path attribute, the user agent will use the
   *             "directory" of the request-uri's path component as the default value.
   *
   *             The user agent will include the cookie in an HTTP request only if the path portion
   *             of the request-uri matches (or is a subdirectory of) the cookie's Path attribute,
   *             where the %x2F ("/") character is interpreted as a directory separator.
   *
   *             Although seemingly useful for isolating cookies between different paths within a given
   *             host, the Path attribute cannot be relied upon for security.
   *
   * @return     path_
   */
  inline const std::string& get_path() const noexcept { return path_; }

  void set_path(const std::string& path);

  /**
   * @brief      Determines if Secure.
   *
   *             [RFC 6265] The Secure attribute limits the scope of the cookie to "secure" channels
   *             (where "secure" is defined by the user agent). When a cookie has the Secure attribute,
   *             the user agent will include the cookie in an HTTP request only if the request is transmitted
   *             over a secure channel (typically HTTP over Transport Layer Security (TLS)).
   *
   *             Although seemingly useful for protecting cookies from active network attackers,
   *             the Secure attribute protects only the cookie's confidentiality. An active network
   *             attacker can overwrite Secure cookies from an insecure channel, disrupting their
   *             integrity.
   *
   * @return     true if Secure, false otherwise.
   */
  inline bool is_secure() const noexcept { return secure_; }

  inline void set_secure(bool secure) noexcept { secure_ = secure; }

  /**
   * @brief      Determines if HttpOnly.
   *
   *             [RFC 6265] The HttpOnly attribute limits the scope of the cookie to HTTP requests.
   *             In particular, the attribute instructs the user agent to omit the cookie
   *             when providing access to cookies via "non-HTTP" APIs (such as a web browser
   *             API that exposes cookies to scripts).
   *
   *             Note that the HttpOnly attribute is independent of the Secure attribute:
   *             a cookie can have both the HttpOnly and the Secure attribute.
   *
   * @return     true if HttpOnly, false otherwise.
   */
  inline bool is_http_only() const noexcept { return http_only_; }

  inline void set_http_only(bool http_only) noexcept { http_only_ = http_only; }

  inline operator std::string() const { return to_string(); }

  std::string to_string() const;

private:
  std::string name_;
  std::string value_;
  std::string expires_;
  int max_age_;   // number of seconds
  std::string domain_;
  std::string path_;
  bool secure_;
  bool http_only_;

  static const std::string C_EXPIRES;
  static const std::string C_MAX_AGE;
  static const std::string C_DOMAIN;
  static const std::string C_PATH;
  static const std::string C_SECURE;
  static const std::string C_HTTP_ONLY;

  bool valid(const std::string& name) const;

  // This must be static to be allowed to use in caseInsCompare method
  static bool caseInsCharCompareN(char a, char b);

  bool caseInsCompare(const std::string& s1, const std::string& s2) const;

  bool valid_option_name(const std::string& option_name) const;

  bool valid_expires_time(const std::string& expires) const noexcept;

};  // < class Cookie

// Because we want a map of Cookies in CookieJar we need to set rules
// for comparing Cookies and deciding if two Cookies are the same or not:

bool operator < (const Cookie& a, const Cookie& b) noexcept;

bool operator == (const Cookie& a, const Cookie& b) noexcept;

std::ostream& operator << (std::ostream& output_device, const Cookie& cookie);

};  // < namespace http

#endif // < HTTP_COOKIE_HPP
