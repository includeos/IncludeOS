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

#include "cookie.hpp"

namespace cookie {

bool operator < (const Cookie& a, const Cookie& b) noexcept {
  return std::hash<std::string>{}(a.get_name() + a.get_value())
    < std::hash<std::string>{}(b.get_name() + b.get_value());
}

bool operator == (const Cookie& a, const Cookie& b) noexcept {
  return std::hash<std::string>{}(a.get_name() + a.get_value())
    == std::hash<std::string>{}(b.get_name() + b.get_value());
}

std::ostream& operator << (std::ostream& output_device, const Cookie& cookie) {
  return output_device << cookie.to_string();
}

const std::string Cookie::C_EXPIRES = "Expires";
const std::string Cookie::C_MAX_AGE = "Max-Age";
const std::string Cookie::C_DOMAIN = "Domain";
const std::string Cookie::C_PATH = "Path";
const std::string Cookie::C_SECURE = "Secure";
const std::string Cookie::C_HTTP_ONLY = "HttpOnly";

bool Cookie::valid(const std::string& name) const {

  std::regex reg{"(.*[\\x00-\\x20\\x22\\x28-\\x29\\x2c\\x2f\\x3a-\\x40\\x5b-\\x5d\\x7b\\x7d\\x7f]+.*)"};

  /* Valid name characters according to the RFC: Any chars excluding
   * control characters and special characters.
   *
   * RFC2068 lists special characters as
   * tspecials      = "(" | ")" | "<" | ">" | "@"
   *                     | "," | ";" | ":" | "\" | <">
   *                     | "/" | "[" | "]" | "?" | "="
   *                     | "{" | "}" | SP | HT
   *
   * SP             = <US-ASCII SP, space (32)>
   * HT             = <US-ASCII HT, horizontal-tab (9)>
   */

  return ((not name.empty()) and (not std::regex_match(name, reg)));
}

bool Cookie::caseInsCharCompareN(char a, char b) {
  return(toupper(a) == toupper(b));
}

bool Cookie::caseInsCompare(const std::string& s1, const std::string& s2) const {
  return((s1.size() == s2.size()) and equal(s1.begin(), s1.end(), s2.begin(), caseInsCharCompareN));
}

bool Cookie::valid_option_name(const std::string& option_name) const {
  return caseInsCompare(option_name, C_EXPIRES) || caseInsCompare(option_name, C_MAX_AGE) ||
    caseInsCompare(option_name, C_DOMAIN) || caseInsCompare(option_name, C_PATH) ||
    caseInsCompare(option_name, C_SECURE) || caseInsCompare(option_name, C_HTTP_ONLY);
}

bool Cookie::valid_expires_time(const std::string& e) const noexcept {
  std::tm tm{};

  //printf("Expires INSIDE VALID_EXPIRES_TIME: %s\n", e);

  // ADDED...:
  std::string expires = e.substr(0);

  //printf("E: %s\n", expires.c_str());

  if (expires.empty())
    return false;

  // NB: strptime changes the expires string

  // Format: Sun, 06 Nov 1994 08:49:37 GMT
  if (strptime(expires.c_str(), "%a, %d %b %Y %H:%M:%S %Z", &tm) not_eq nullptr)
    return true;

  // Format: Sunday, 06-Nov-94 08:49:37 GMT
  if (strptime(expires.c_str(), "%a, %d-%b-%y %H:%M:%S %Z", &tm) not_eq nullptr)
    return true;

  // Format: Sun Nov 6 08:49:37 1994
  if(strptime(expires.c_str(), "%a %b %d %H:%M:%S %Y", &tm) not_eq nullptr)
    return true;

  return false;
}

Cookie::Cookie(const std::string& name, const std::string& value) {
  if(not valid(name))
    throw CookieException{"Invalid name (" + name + ") of cookie!"};

  // value can be empty
  if(not value.empty() and not valid(value))
    throw CookieException{"Invalid value (" + value + ") of cookie!"};

  name_ = name;
  value_ = value;

  // set default values:
  expires_ = "";
  max_age_ = -1;  // 0 deletes the cookie right away
  domain_ = "";
  path_ = "/";
  secure_ = false;
  http_only_ = false;
}

Cookie::Cookie(const std::string& name, const std::string& value, const std::vector<std::string>& options)
  : Cookie{name, value}
{
  // Check that the vector has an even number of elements (key, value)
  if(options.size() % 2 not_eq 0)
    throw CookieException{"Invalid number of elements in cookie's options vector!"};

  // for-loop on vector - set input values from vector:
  for(size_t i = 0; i < options.size(); i += 2) {
    std::string nm = options[i];

    if(!valid_option_name(nm))
      throw CookieException{"Invalid name (" + nm + ") of cookie option!"};

    std::string val = options[i+1];

    if(caseInsCompare(nm, C_EXPIRES)) {

      // printf("Val expires in constructor: %s\n", val.c_str());

      set_expires(val);
    } else if(caseInsCompare(nm, C_MAX_AGE)) {
      int age = std::stoi(val); // Can throw exception (invalid_argument or out_of_range)

      if(age < 0)
        throw CookieException{"Invalid max-age attribute (" + val + ") of cookie! Negative number of seconds not allowed."};

      set_max_age(age);
    } else if(caseInsCompare(nm, C_DOMAIN)) {
      set_domain(val);
    } else if(caseInsCompare(nm, C_PATH)) {
      set_path(val);
    } else if(caseInsCompare(nm, C_SECURE)) {
      bool s = (caseInsCompare(val, "true")) ? true : false;
      set_secure(s);
    } else if(caseInsCompare(nm, C_HTTP_ONLY)) {
      bool s = (caseInsCompare(val, "true")) ? true : false;
      set_http_only(s);
    }
  }
}

const std::string& Cookie::get_name() const noexcept {
  return name_;
}

const std::string& Cookie::get_value() const noexcept {
  return value_;
}

void Cookie::set_value(const std::string& value) {
  // value can be empty
  if(not value.empty() and not valid(value))
    throw CookieException{"Invalid value (" + value + ") of cookie!"};

  value_ = value;
}

const std::string& Cookie::get_expires() const noexcept {

  //printf("Expires in get: %s\n", expires_.c_str());

  return expires_;
}

// void Cookie::set_expires(const ExpiryDate& expires) {
void Cookie::set_expires(const std::string& expires) {

  //printf("Expires in set before: %s\n", expires.c_str());

  expires_ = expires;

  //printf("Expires in set after: %s\n", expires_.c_str());

  // printf("Expires in set before test: %s and %s\n", expires_.c_str(), expires.c_str());

  if(not valid_expires_time(expires)) { // valid_expires_time changes expires

    // printf("NOT VALID EXPIRES TIME: %s\n", expires);

    expires_ = "";
    throw CookieException{"Invalid expires attribute (" + expires + ") of cookie!"};
  }

  // Not necessary to change max_age_ because every browser uses the Expires attribute
  // and persists it properly (Max-Age is ignored if Expires is set).
}

int Cookie::get_max_age() const noexcept {
  return max_age_;
}

void Cookie::set_max_age(int max_age) noexcept {
  expires_.clear(); // TODO: Do this or not? Not every browser supports the Max-Age
                    // attribute, but if it does, the Expires attribute is ignored.
                    // Internet Explorer does not support the Max-Age attribute.

  if(max_age < 0)
    throw CookieException{"Invalid max-age attribute (" + std::to_string(max_age) + ") of cookie! Negative number of seconds not allowed."};

  max_age_ = max_age;
}

const std::string& Cookie::get_domain() const noexcept {
  return domain_;
}

void Cookie::set_domain(const std::string& domain) {
  if(domain.empty()) {
    domain_ = "";
    return;
  }

  // [RFC 6265] If the first character of the attribute-value string is ".", let
  // cookie-domain be the attribute-value without the leading "." character.
  if(domain.at(0) == '.')
    domain_ = domain.substr(1);
  else
    domain_ = domain;

  // [RFC 6265] Convert the cookie-domain to lower case.
  std::transform(domain_.begin(), domain_.end(), domain_.begin(), ::tolower);
}

const std::string& Cookie::get_path() const noexcept {
  return path_;
}

void Cookie::set_path(const std::string& path) {
  // [RFC 6265 5.2.4]
  if(path.empty()) {
    path_ = "/";
    return;
  }

  // [RFC 6265 5.2.4]
  if(path.at(0) not_eq '/')
    throw CookieException{"Invalid path (" + path + ") of cookie! It must start with a / character."};

  std::for_each(path.begin(), path.end(), [&path](const char c) {
    if(::iscntrl(c) or (c == ';')) {
      throw CookieException{"Invalid path (" + path + ") of cookie!"};
    }
  });

  path_ = path;
}

bool Cookie::is_secure() const noexcept {
  return secure_;
}

void Cookie::set_secure(bool secure) noexcept {
  secure_ = secure;
}

bool Cookie::is_http_only() const noexcept {
  return http_only_;
}

void Cookie::set_http_only(bool http_only) noexcept {
  http_only_ = http_only;
}

Cookie::operator std::string() const {
  return to_string();
}

std::string Cookie::to_string() const {
  std::string cookie_string = name_ + "=" + value_;

  // expires_ is empty because of valid_expires_time-method ??
  // printf("Expires in to_string: %s\n", expires_.c_str());

  if(not expires_.empty())
    cookie_string += "; " + C_EXPIRES + "=" + expires_;

  if(max_age_ not_eq -1)
    cookie_string += "; " + C_MAX_AGE + "=" + std::to_string(max_age_);

  if(not domain_.empty())
    cookie_string += "; " + C_DOMAIN + "=" + domain_;

  if(not path_.empty())
    cookie_string += "; " + C_PATH + "=" + path_;

  if(secure_)
    cookie_string += "; " + C_SECURE;

  if(http_only_)
    cookie_string += "; " + C_HTTP_ONLY;

  return cookie_string;
}

};  // < namespace cookie
