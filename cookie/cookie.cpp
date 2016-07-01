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

/*In cookie.hpp:
#include <regex>
#include <string>
#include <vector>
#include <cctype>
#include <utility>
#include <ostream>
#include <algorithm>
#include <functional>*/

using namespace cookie;

/*
Cookie::Cookie(std::string& key, std::string& value)
  : key_(key), value_(value) {

}

Cookie::Cookie(std::string& key, std::string& value, std::string& options)
  : key_(key), value_(value), options_(options) {

}

Cookie::~Cookie() {

}
*/

const std::string Cookie::no_entry_value_;

inline static bool equiv_strs(std::string& s1, std::string& s2) {
  std::transform(s1.begin(), s1.end(), s1.begin(), ::tolower());
  std::transform(s2.begin(), s2.end(), s2.begin(), ::tolower());
  return s1 == s2;
}

inline auto Cookie::find(const std::string& keyword) const {
  return std::find_if(data_.begin(), data_.end(), [&keyword](const auto& k) {
    return equiv_strs(k.first, keyword);
  });
}

inline Cookie::Cookie(const std::string& data, Parser parser) : data_{}, parser_{parser} {
  parse(data);
}

inline const std::string& Cookie::name() const {
  return data_.at(0).first;
}

inline const std::string& Cookie::value() const {
  return data_.at(0).second;
}

inline const std::string& Cookie::expires() const {
  auto it = find("Expires");
  return (it not_eq data_.end()) ? it->second : no_entry_value_;
}

inline const std::string& Cookie::max_age() const {
  auto it = find("Max-Age");
  return (it not_eq data_.end()) ? it->second : no_entry_value_;
}

inline const std::string& Cookie::domanin() const {
  auto it = find("Domain");
  return (it not_eq data_.end()) ? it->second : no_entry_value_;
}

inline const std::string& Cookie::path() const {
  auto it = find("Path");
  return (it not_eq data_.end()) ? it->second : no_entry_value_;
}

inline const bool Cookie::is_secure() const noexcept {
  return find("Secure") not_eq data_.end();
}

inline const bool Cookie::is_http_only() const noexcept {
  return find("HttpOnly") not_eq data_.end();
}

// Fill in
std::string Cookie::to_string() const {
  std::string cookieString = "";

  cookieString += "";

  return cookieString;
}

Cookie::operator std::string () const {
  return to_string();
}

inline void Cookie::parse(const std::string& data) {
  if(parser_) {
    data_ = parser_(data);
  } else {
    static const std::regex pattern{"[^;]+"};
    auto position = std::sregex_iterator(data.begin(), data.end(), pattern);
    auto end = std::sregex_iterator();

    // And more ...

  }
}

inline std::ostream& operator << (std::ostream& output_device, const Cookie& cookie) {
  return output_device << cookie.to_string();
}

// ?
std::string Cookie::serialize(std::string& key, std::string& value) {
  //if(key.empty())

  std::string cookieString = "";

  cookieString += "";

  return cookieString;
}

// ?
std::string Cookie::serialize(std::string& key, std::string& value, std::string& options) {

}

// ?
bool Cookie::deserialize() {

}

// ?
std::shared_ptr<Cookie> Cookie::parse(std::string& str, std::string& options) {
  std::shared_ptr<Cookie> cookie(new Cookie());

  return cookie;
}
