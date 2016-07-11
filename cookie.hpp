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

#ifndef COOKIE_HPP
#define COOKIE_HPP

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
#include <chrono>

#include "time.hpp"

namespace cookie {

  /*struct Serializable {
    virtual void serialize(std::string& key, std::string& value) const = 0;
    virtual void serialize(std::string& key, std::string& value, std::string& options) const = 0;
    virtual bool deserialize() = 0;
  };*/

  class CookieException : public std::exception {
    const char* msg;
    CookieException(){}

  public:
    CookieException(const char* s) throw() : msg(s){}
    const char* what() const throw() { return msg; }
  };  // < class CookieException

  class Cookie {
  private:
    //using CookieData = std::vector<std::pair<std::string, std::string>>;
    //using Parser = std::function<CookieData(const std::string&)>;
    using ExpiryDate = std::chrono::system_clock::time_point;

  public:

    /* Constructor and destructor necessary?:
    Cookie(std::string& name, std::string& value);
    Cookie(std::string& name, std::string& value, std::string& options);
    ~Cookie();
    */

    explicit Cookie(const std::string& name, const std::string& value);

    explicit Cookie(const std::string& name, const std::string& value, const std::vector<std::string>& options);

    //Cookie(const std::string& data, Parser parser = nullptr);

    Cookie(const Cookie&) = default;

    Cookie(Cookie&&) noexcept = default;

    Cookie& operator = (const Cookie&) = default;

    Cookie& operator = (Cookie&&) = default;

    ~Cookie() = default;

    const std::string& get_name() const noexcept;

    const std::string& get_value() const noexcept;

    void set_value(const std::string& value);

    /**
     * @brief      [RFC 6265] The Expires attribute indicates the maximum lifetime of the cookie,
     *             represented as the date and time at which the cookie expires. The user agent
     *             is not required to retain the cookie until the specified date has passed. In fact,
     *             user agents often evict cookies due to memory pressure or privacy concerns.
     *
     *             { function_description }
     *
     * @return     { description_of_the_return_value }
     */
    const std::string& get_expires() const; // date and time

    // void set_expires(const std::string& expires);

    /**
     * @brief      Sets the expires_ attribute.
     *
     * @param[in]  expires  The expiry date for the cookie (if set in the past,
     *             the cookie will be deleted/cleared from the browser)
     */
    void set_expires(const ExpiryDate& expires);

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
     *             { function_description }
     *
     * @return     { description_of_the_return_value }
     */
    std::chrono::seconds get_max_age() const noexcept;

    void set_max_age(std::chrono::seconds max_age) noexcept;

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
     *             { function_description }
     *
     * @return     { description_of_the_return_value }
     */
    const std::string& get_domain() const noexcept;

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
     *             { function_description }
     *
     * @return     { description_of_the_return_value }
     */
    const std::string& get_path() const noexcept;

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
     * @return     True if Secure, False otherwise.
     */
    bool is_secure() const noexcept;

    void set_secure(bool secure) noexcept;

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
     * @return     True if HttpOnly, False otherwise.
     */
    bool is_http_only() const noexcept;

    void set_http_only(bool http_only) noexcept;

    bool is_valid() const noexcept;

    operator bool() const noexcept;

    operator std::string() const;

    std::string to_string() const;

    // TODO: Alternative: have serialize-method in CookieParser that calls the cookie's to_string()
    // serialize-methods called by CookieParser
    /**
     * @brief      Serialize cookie
     *
     * @param      name   The name
     * @param      value  The value
     *
     * @return     Cookie-string to be set in http header
     */
    std::string serialize() const;

  private:
    std::string name_;
    std::string value_;
    std::string expires_;
    std::chrono::seconds max_age_;
    std::string domain_;
    std::string path_;
    bool secure_;
    bool http_only_;

    std::chrono::system_clock::time_point time_created_;

    //CookieData data_;
    //Parser parser_;

    //static const std::string C_NO_ENTRY_VALUE;
    static const std::string C_EXPIRES;
    static const std::string C_MAX_AGE;
    static const std::string C_DOMAIN;
    static const std::string C_PATH;
    static const std::string C_SECURE;
    static const std::string C_HTTP_ONLY;

    /* This must be static to be allowed to use as part of std::equal in icompare method
    static bool icompare_pred(unsigned char a, unsigned char b);

    bool icompare(const std::string& a, const std::string& b) const;*/

    // Move to CookieParser ?
    // Cookie& parse(const std::string& cookie_string);

    //auto find(const std::string& keyword) const;

    bool valid(const std::string& name) const;

    // This must be static to be allowed to use in caseInsCompare method
    static bool caseInsCharCompareN(char a, char b);

    bool caseInsCompare(const std::string& s1, const std::string& s2) const;

    bool valid_option_name(std::string& option_name) const;

    bool expired() const;

  };  // < class Cookie

};  // < namespace cookie

#endif
