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

//const std::string Cookie::C_NO_ENTRY_VALUE;
const std::string Cookie::C_EXPIRES = "Expires";
const std::string Cookie::C_MAX_AGE = "Max-Age";
const std::string Cookie::C_DOMAIN = "Domain";
const std::string Cookie::C_PATH = "Path";
const std::string Cookie::C_SECURE = "Secure";
const std::string Cookie::C_HTTP_ONLY = "HttpOnly";

/*inline bool Cookie::icompare_pred(unsigned char a, unsigned char b) {
  return std::tolower(a) == std::tolower(b);
}

inline bool Cookie::icompare(const std::string& a, const std::string& b) const {
  if(a.length() == b.length())
    return std::equal(b.begin(), b.end(), a.begin(), icompare_pred);
  else
    return false;
}*/

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

inline bool Cookie::caseInsCharCompareN(char a, char b) {
  return(toupper(a) == toupper(b));
}

bool Cookie::caseInsCompare(const std::string& s1, const std::string& s2) const {
  return((s1.size() == s2.size()) and equal(s1.begin(), s1.end(), s2.begin(), caseInsCharCompareN));
}

bool Cookie::valid_option_name(std::string& option_name) const {
  return caseInsCompare(option_name, C_EXPIRES) || caseInsCompare(option_name, C_MAX_AGE) ||
    caseInsCompare(option_name, C_DOMAIN) || caseInsCompare(option_name, C_PATH) ||
    caseInsCompare(option_name, C_SECURE) || caseInsCompare(option_name, C_HTTP_ONLY);

  /* // boost::iequals ignore case in comparing strings
  return icompare(option_name, C_EXPIRES) || icompare(option_name, C_MAX_AGE) || icompare(option_name, C_DOMAIN) ||
    icompare(option_name, C_PATH) || icompare(option_name, C_SECURE) || icompare(option_name, C_HTTP_ONLY); */
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
  // TODO: Better solution than throwing exception here?
  if(!valid(name) or !valid(value))
    throw CookieException{"Invalid name or value of cookie!"};

  name_ = name;
  value_ = value;

  //time_created_ = std::chrono::system_clock::now(); // TODO: trouble with chrono (undefined reference to clock_gettime)

  // set default values:
  expires_ = "";
  max_age_ = std::chrono::seconds(0);
  domain_ = "";
  path_ = "";
  secure_ = false;
  http_only_ = false;
}

// TODO: Better to just have the constructor Cookie(name, value) and have set methods that can be called for every option?
Cookie::Cookie(const std::string& name, const std::string& value, const std::vector<std::string>& options) {
  Cookie{name, value};

  // for loop on vector - set input values from vector:
  for(size_t i = 0; i < options.size(); i += 2) {
    std::string nm = options[i];

    if(!valid_option_name(nm))
      throw CookieException{"Invalid name of cookie option."};

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

/*inline Cookie::Cookie(const std::string& data, Parser parser) : data_{}, parser_{parser} {

  // parse(name, value, options) or similar instead of constructor?
  // Need to validate the data string!

  parse(data);
}*/

const std::string& Cookie::get_name() const noexcept {
  //return data_.at(0).first;

  return name_;
}

const std::string& Cookie::get_value() const noexcept {
  //return data_.at(0).second;

  return value_;
}

void Cookie::set_value(const std::string& value) {
  if(!valid(value))
    throw CookieException{"Invalid value of cookie!"};

  value_ = value;
}

const std::string& Cookie::get_expires() const {
  /*auto it = find(C_EXPIRES);
  return (it not_eq data_.end()) ? it->second : C_NO_ENTRY_VALUE;*/

  return expires_;
}

/*
void Cookie::set_expires(const std::string& expires) {
  expires_ = expires;
}*/

void Cookie::set_expires(const ExpiryDate& expires) {

  // need to #include "time.hpp" to use (rico)

  /* TODO: Trouble chrono: undefined reference to clock_gettime
  auto exp_date = std::chrono::system_clock::to_time_t(expires);
  //max_age_ = 0s;
  max_age_ = std::chrono::seconds(0);
  expires_ = time::from_time_t(exp_date);
  */
}

std::chrono::seconds Cookie::get_max_age() const noexcept {
  /*auto it = find(C_MAX_AGE);
  return (it not_eq data_.end()) ? it->second : C_NO_ENTRY_VALUE;*/

  return max_age_;
}

void Cookie::set_max_age(std::chrono::seconds max_age) noexcept {
  expires_.clear();
  max_age_ = max_age;
}

const std::string& Cookie::get_domain() const noexcept {
  /*auto it = find(C_DOMAIN);
  return (it not_eq data_.end()) ? it->second : C_NO_ENTRY_VALUE;*/

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
    path_ = '/';
    return;
  }

  // TODO: A custom regex/check for path ?
  if(!valid(path))
    throw CookieException{"Invalid path!"};

  /* Alt. (add more?):
   * std::for_each(path.begin(), path.end(), [&path](const char c) {
   *  if(::iscntrl(c) or (c == ';'))
   *    throw CookieException{"Invalid path!"};
   * });
   */

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
