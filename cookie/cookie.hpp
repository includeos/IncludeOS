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

//#include "attribute.hpp"

// Create cookie here??? (add_header(...) til Response og Request) ELLER UTVIKLER OPPRETTE COOKIES SELV: res.setHeader('Set-Cookie', cookie.serialize()))
// Se screenshot Plan_Annika hvor det står øverst var cookie = require(´cookie´): Det er cookie-middlewaret som har metoden serialize (blir cookie_parser.hpp), men
// her opprettes en cookie ved å gjøre en metode på res (response) (res.setHeader(...)) - hva ønsker vi??
// Middleware for kun å parse og så metoder i Response og Request for å opprette cookies???? Er det sånn det er løst i Node.js??

#include <regex>
#include <string>
#include <vector>
#include <cctype>
#include <utility>
#include <ostream>
#include <algorithm>
#include <functional>

namespace cookie {

  /*struct Serializable {
    virtual void serialize(std::string& key, std::string& value) const = 0;
    virtual void serialize(std::string& key, std::string& value, std::string& options) const = 0;
    virtual bool deserialize() = 0;
  };*/

  class Cookie {
  private:
    using CookieData = std::vector<std::pair<std::string, std::string>>;
    using Parser = std::function<CookieData(const std::string&)>;
        // std::function: a function that returns CookieData and takes a string as parameter

  public:

    /* Constructor and destructor necessary?:
    Cookie(std::string& key, std::string& value);
    Cookie(std::string& key, std::string& value, std::string& options);
    ~Cookie();
    */

    explicit Cookie(const std::string& data, Parser parser = nullptr);

    const std::string& name() const;

    const std::string& value() const;

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
    const std::string& expires() const; // date and time

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
    const std::string& max_age() const;

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
    const std::string& domain() const;

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
    const std::string& path() const;

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

    std::string to_string() const;

    operator std::string () const;

    // serialize-methods called by CookieParser

    // ?
    const std::string serialize(std::string& key, std::string& value); // Return cookie-string to be set in header

    // ?
    const std::string serialize(std::string& key, std::string& value, std::string& options);

    // ?
    bool deserialize();

    // ?
    std::shared_ptr<Cookie> parse(std::string& str, std::string& options);  // Parse str to a Cookie-object ?

  private:
    /*std::string key_;
    std::string value_;
    std::string options_;*/

    CookieData data_;
    Parser parser_;

    static const std::string no_entry_value_;

    // ?
    void parse(const std::string& data);

    auto find(const std::string& keyword) const;
  };

};  // < namespace cookie


#endif
