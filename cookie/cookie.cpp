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

// TODO: Test regex and check what values are valid for name and value in a cookie
bool Cookie::valid(const std::string& name) const {

  std::regex reg{"([\u0009\u0020-\u007e\u0080-\u00ff]+)"};
  // TODO: Regex source: https://github.com/pillarjs/cookies/blob/master/lib/cookies.js:
  /**
   * RegExp to match field-content in RFC 7230 sec 3.2
   *
   * field-content = field-vchar [ 1*( SP / HTAB ) field-vchar ]
   * field-vchar   = VCHAR / obs-text
   * obs-text      = %x80-FF
   */

  // %x21 / %x23-2B / %x2D-3A / %x3C-5B / %x5D-7E
  // % missing:
  //std::regex reg("([a-zA-Z0-9!#\\$&'\\*\\+\\-\\.\\^_`\\|~]+)");

  return (name.empty() or not (std::regex_match(name, reg))) ? false : true;
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

bool Cookie::expired() const {

  /* need to #include "time.hpp" to use (rico) + TODO: trouble with chrono (undefined reference to clock_gettime)
  const auto now = std::chrono::system_clock::now();
  const auto delta = std::chrono::duration_cast<std::chrono::seconds>(now - time_created_);

  if(not expires_.empty()) {
    const auto expiry_time = std::chrono::system_clock::from_time_t(time::to_time_t(expires_));

    return now > expiry_time;
  }

  return delta > max_age_;*/

  return true;
}

Cookie::Cookie(const std::string& name, const std::string& value) {
  if(not valid(name))
    throw CookieException{"Invalid name (" + name + ") of cookie!"};

  // value can be empty
  if(not value.empty() and not valid(value))
    throw CookieException{"Invalid value (" + value + ") of cookie!"};

  name_ = name;
  value_ = value;

  //time_created_ = std::chrono::system_clock::now(); // TODO: trouble with chrono (undefined reference to clock_gettime)

  // set default values:
  expires_ = "";
  max_age_ = std::chrono::seconds(0);
  domain_ = "";
  path_ = "/";
  secure_ = false;
  http_only_ = false;
}

Cookie::Cookie(const std::string& name, const std::string& value, const std::vector<std::string>& options)
  : Cookie{name, value}
{
  // for loop on vector - set input values from vector:
  for(size_t i = 0; i < options.size(); i += 2) {
    std::string nm = options[i];

    if(!valid_option_name(nm))
      throw CookieException{"Invalid name (" + nm + ") of cookie option!"};

    std::string val = options[i+1];

    if(caseInsCompare(nm, C_EXPIRES)) {
      // TODO: Change to set_expires(val); when method finished/working
      expires_ = val;
    } else if(caseInsCompare(nm, C_MAX_AGE)) {
      set_max_age(std::chrono::seconds(atoi(val.c_str())));
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

const std::string& Cookie::get_expires() const {
  return expires_;
}

/*
void Cookie::set_expires(const std::string& expires) {
  expires_ = expires;
}*/

//void Cookie::set_expires(const ExpiryDate& expires) {
void Cookie::set_expires(const std::string& expires) {

  // TODO: Input validation or const ExpiryDate& expires

  // need to #include "time.hpp" to use (rico)

  /* TODO: Trouble chrono: undefined reference to clock_gettime
  auto exp_date = std::chrono::system_clock::to_time_t(expires);
  //max_age_ = 0s;
  max_age_ = std::chrono::seconds(0);
  expires_ = time::from_time_t(exp_date);
  */

  expires_ = expires;
}

std::chrono::seconds Cookie::get_max_age() const noexcept {
  return max_age_;
}

void Cookie::set_max_age(std::chrono::seconds max_age) noexcept {
  expires_.clear();
  max_age_ = max_age;
}

const std::string& Cookie::get_domain() const noexcept {
  return domain_;
}

void Cookie::set_domain(const std::string& domain) {
  // TODO: Check input parameter?

  domain_ = domain;
}

const std::string& Cookie::get_path() const noexcept {
  return path_;
}

void Cookie::set_path(const std::string& path) {
  if(path.empty()) {
    path_ = "/";
    return;
  }

  std::for_each(path.begin(), path.end(), [&path](const char c) {
    if(::iscntrl(c) or (c == ';')) {
      throw CookieException{"Invalid path (" + path + ")!"};
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

bool Cookie::is_valid() const noexcept {
  return not expired();
}

Cookie::operator bool() const noexcept {
  return is_valid();
}

Cookie::operator std::string() const {
  return to_string();
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
}

};  // < namespace cookie
