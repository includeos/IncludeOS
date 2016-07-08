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

// #include "time.hpp"

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

    Cookie(const std::string& name, const std::string& value);

    Cookie(const std::string& name, const std::string& value, const std::vector<std::string>& options);

    //Cookie(const std::string& data, Parser parser = nullptr);

    ~Cookie() = default;

    const std::string& get_name() const;

    const std::string& get_value() const;

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
    const std::chrono::seconds& get_max_age() const;

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
    const std::string& get_domain() const;

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
    const std::string& get_path() const;

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

    operator std::string() const;

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

    static const std::string C_NO_ENTRY_VALUE;
    static const std::string C_EXPIRES;
    static const std::string C_MAX_AGE;
    static const std::string C_DOMAIN;
    static const std::string C_PATH;
    static const std::string C_SECURE;
    static const std::string C_HTTP_ONLY;

    // This must be static to be allowed to use as part of std::equal in icompare method
    static bool icompare_pred(unsigned char a, unsigned char b);

    bool icompare(const std::string& a, const std::string& b) const;

    // Move to CookieParser ?
    // Cookie& parse(const std::string& cookie_string);

    //auto find(const std::string& keyword) const;

    bool valid(const std::string& name) const;

    bool valid_option_name(std::string& option_name) const;

    bool expired() const;

  };  // < class Cookie

  //------------------------- Implementation details ------------------------

  const std::string Cookie::C_NO_ENTRY_VALUE;
  const std::string Cookie::C_EXPIRES = "Expires";
  const std::string Cookie::C_MAX_AGE = "Max-Age";
  const std::string Cookie::C_DOMAIN = "Domain";
  const std::string Cookie::C_PATH = "Path";
  const std::string Cookie::C_SECURE = "Secure";
  const std::string Cookie::C_HTTP_ONLY = "HttpOnly";

  inline bool Cookie::icompare_pred(unsigned char a, unsigned char b) {
    return std::tolower(a) == std::tolower(b);
  }

  inline bool Cookie::icompare(const std::string& a, const std::string& b) const {
    if(a.length() == b.length())
      return std::equal(b.begin(), b.end(), a.begin(), icompare_pred);
    else
      return false;
  }

  /* Move to CookieParser ?
  Cookie& Cookie::parse(const std::string& cookie_string) {
    // add data to data_ (CookieData)
*/
    /*if(parser_) {
      data_ = parser_(data);
    } else {*/
/*
    static const std::regex pattern{"[^;]+"};
    auto position = std::sregex_iterator(data.begin(), data.end(), pattern);
    auto end = std::sregex_iterator();

    for (std::sregex_iterator i = position; i != end; ++i) {
        std::smatch pair = *i;
        std::string pair_str = pair.str();

        // Remove all empty spaces:
        pair_str.erase(std::remove(pair_str.begin(), pair_str.end(), ' '), pair_str.end());
*/
        /*Alt.:
        vector<std::string> v;
        boost::split(v, pair_str, boost::is_any_of("="));
        data_.push_back(std::make_pair(v.at(0), v.at(1)));*/
/*
        size_t pos = pair_str.find("=");
        std::string name = pair_str.substr(0, pos);
        std::string value = pair_str.substr(pos + 1);

        //data_.push_back(std::make_pair(name, value));
    }

    //}
  }
*/

  /*inline auto Cookie::find(const std::string& keyword) const {
    return std::find_if(data_.begin(), data_.end(), [&keyword](const auto& k){
      return std::equal(k.first.begin(), k.first.end(), keyword.begin(), keyword.end(),
             [](const auto a, const auto b) { return ::tolower(a) == ::tolower(b);
      });
    });
  }*/

  // TODO: Test regex
  bool Cookie::valid(const std::string& name) const {
    // %x21 / %x23-2B / %x2D-3A / %x3C-5B / %x5D-7E

    // % missing
    std::regex reg("([a-zA-Z!#\\$&'\\*\\+\\-\\.\\^_`\\|~]+)");
    //std::regex reg("/^[\u0009\u0020-\u007e\u0080-\u00ff]+$/");

    return (name.empty() || !(std::regex_match(name, reg))) ? false : true;
  }

  bool Cookie::valid_option_name(std::string& option_name) const {
    // boost::iequals ignore case in comparing strings
    return icompare(option_name, C_EXPIRES) || icompare(option_name, C_MAX_AGE) || icompare(option_name, C_DOMAIN) ||
      icompare(option_name, C_PATH) || icompare(option_name, C_SECURE) || icompare(option_name, C_HTTP_ONLY);
  }

  bool Cookie::expired() const {

    /* need to #include "time.hpp" to use (rico)

    if(not expires_.empty()) {
      auto expiry_time = time::to_time_t(expires_);

      if(expiry_time >= std::time(nullptr))
        return true;
    } else {
      const auto now = std::chrono::system_clock::now();
      const auto delta = std::chrono::duration_cast<std::chrono::seconds>(now - time_created_);
    }

    return false;
    */

    return true;
  }

  Cookie::Cookie(const std::string& name, const std::string& value) {

    // TODO: Better solution than throwing exception here?
    if(!valid(name) or !valid(value))
        throw CookieException("Invalid name or value of cookie!");

      name_ = name;
      value_ = value;
  }

  Cookie::Cookie(const std::string& name, const std::string& value, const std::vector<std::string>& options) {
      Cookie{name, value};

      // for loop on vector - set values:


  }

  /*inline Cookie::Cookie(const std::string& data, Parser parser) : data_{}, parser_{parser} {

    // parse(name, value, options) or similar instead of constructor?
    // Need to validate the data string!

    parse(data);
  }*/

  const std::string& Cookie::get_name() const {
    //return data_.at(0).first;

    return name_;
  }

  const std::string& Cookie::get_value() const {
    //return data_.at(0).second;

    return value_;
  }

  const std::string& Cookie::get_expires() const {
    /*auto it = find(C_EXPIRES);
    return (it not_eq data_.end()) ? it->second : C_NO_ENTRY_VALUE;*/

    return expires_;
  }

  void Cookie::set_expires(const ExpiryDate& expires) {

    /* need to #include "time.hpp" to use (rico)

    using namespace std::literals::chrono_literals;
    auto exp_date = std::chrono::system_clock::to_time_t(expires);
    max_age_ = 0s;
    expires_ = time::from_time_t(exp_date);*/

  }

  const std::chrono::seconds& Cookie::get_max_age() const {
    /*auto it = find(C_MAX_AGE);
    return (it not_eq data_.end()) ? it->second : C_NO_ENTRY_VALUE;*/

    return max_age_;
  }

  const std::string& Cookie::get_domain() const {
    /*auto it = find(C_DOMAIN);
    return (it not_eq data_.end()) ? it->second : C_NO_ENTRY_VALUE;*/

    return domain_;
  }

  const std::string& Cookie::get_path() const {
    /*auto it = find(C_PATH);
    return (it not_eq data_.end()) ? it->second : C_NO_ENTRY_VALUE;*/

    return path_;
  }

  bool Cookie::is_secure() const noexcept {
    //return find(C_SECURE) not_eq data_.end();

    return secure_;
  }

  bool Cookie::is_http_only() const noexcept {
    //return find(C_HTTP_ONLY) not_eq data_.end();

    return http_only_;
  }

  std::string Cookie::to_string() const {
    std::ostringstream cookie_stream;

    cookie_stream << name_ << "=" << value_;

    if(not expires_.empty())
      cookie_stream << "; " << C_EXPIRES << "=" << expires_;

    if(max_age_.count())
      cookie_stream << "; " << C_MAX_AGE << "=" << max_age_.count();

    if(not domain_.empty())
      cookie_stream << "; " << C_DOMAIN << "=" << domain_;

    if(not path_.empty())
      cookie_stream << "; " << C_PATH << "=" << path_;

    if(secure_)
      cookie_stream << "; " << C_SECURE;

    if(http_only_)
      cookie_stream << "; " << C_HTTP_ONLY;

    return cookie_stream.str();

    return "";
  }

  Cookie::operator std::string() const {
    return to_string();
  }

  std::ostream& operator<< (std::ostream& output_device, const Cookie& cookie) {
    return output_device << cookie.to_string();
  }

  std::string Cookie::serialize() const {
    return to_string();
  }

  /*const std::string& Cookie::serialize(const std::string& name, const std::string& value) {

    // TODO: TRY CATCH WHEN CALLING THIS METHOD

    // check if valid parameter values:
    if(!valid(name) || !valid(value)) {

      printf("Name and value: %s and %s", name.c_str(), value.c_str());

      const char* message = ("Invalid name " + name + " or value " + value + " of cookie!").c_str();
      throw CookieException{message};
    }

    // create CookieData or just string??
    // ADD NAME AND VALUE TO COOKIEDATA ?? CLEAR COOKIEDATA FIRST??
    // A standard serialize method in node.js is just a function: no class with private attributes
    // and therefore just return string and nothing else
    //data_.clear();
    //data_.push_back(std::make_pair(name, value));

    std::string s{name + "=" + value};


  }

  const std::string& Cookie::serialize(const std::string& name, const std::string& value, const std::vector<std::string>& options) {

    // TODO: TRY CATCH
    std::string cookieString = serialize(name, value);

    // add options to the string and data_ (CookieData) ??

    if(options.empty())
      return cookieString;

    // Test all values in options with valid-method?
    // Find option-names (Domain and more )
    // Iterate options?

    for(size_t i = 0; i < options.size(); i += 2) {
      std::string nm = options[i];

      if(!valid_option_name(nm)) {
        const char* message = (nm + " is not a valid option!").c_str();
        throw CookieException{message};
      }

      std::string val = options[i+1];
*/
      /* No need to validate values..? Browser validates the values?
      if(!valid(val)) {
        const char* message = ("Invalid value " + val + " of cookie!").c_str();
        throw CookieException{message};
      }*/
/*
      cookieString += "; " + nm + "=" + val;

      data_.push_back(std::make_pair(nm, val));
    }

    //cookieString += "; " + "";
  }
*/

};  // < namespace cookie

#endif
