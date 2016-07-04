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
#include <string>
//#include <boost/algorithm/string.hpp>

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

inline auto Cookie::find(const std::string& keyword) const {
  return std::find_if(data_.begin(), data_.end(), [&keyword](const auto& k){
    return std::equal(k.first.begin(), k.first.end(), keyword.begin(), keyword.end(),
           [](const auto a, const auto b) { return ::tolower(a) == ::tolower(b);
    });
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

inline const std::string& Cookie::domain() const {
  auto it = find("Domain");
  return (it not_eq data_.end()) ? it->second : no_entry_value_;
}

inline const std::string& Cookie::path() const {
  auto it = find("Path");
  return (it not_eq data_.end()) ? it->second : no_entry_value_;
}

inline bool Cookie::is_secure() const noexcept {
  return find("Secure") not_eq data_.end();
}

inline bool Cookie::is_http_only() const noexcept {
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
  // add data to data_ (CookieData)

  if(parser_) {
    data_ = parser_(data);
  } else {
    static const std::regex pattern{"[^;]+"};
    auto position = std::sregex_iterator(data.begin(), data.end(), pattern);
    auto end = std::sregex_iterator();

    for (std::sregex_iterator i = position; i != end; ++i) {
        std::smatch pair = *i;
        std::string pair_str = pair.str();

        // Remove all empty spaces:
        pair_str.erase(std::remove(pair_str.begin(), pair_str.end(), ' '), pair_str.end());

        /*Alt.:
        vector<std::string> v;
        boost::split(v, pair_str, boost::is_any_of("="));
        data_.push_back(std::make_pair(v.at(0), v.at(1)));*/

        std::size_t pos = pair_str.find("=");
        std::string key = pair_str.substr(0, pos);
        std::string val = pair_str.substr(pos + 1);

        data_.push_back(std::make_pair(key, val));

        //printf("CookieData data_: %s = %s\n", key.c_str(), val.c_str());
    }
  }
}

inline std::ostream& operator << (std::ostream& output_device, const Cookie& cookie) {
  return output_device << cookie.to_string();
}

// ?
const std::string Cookie::serialize(std::string& key, std::string& value) {
  //if(key.empty())

  std::string cookieString = "";

  cookieString += "";

  return cookieString;
}

// ?
const std::string Cookie::serialize(std::string& key, std::string& value, std::string& options) {

}

// ?
bool Cookie::deserialize() {

}

// ?
std::shared_ptr<Cookie> Cookie::parse(std::string& str, std::string& options) {
  /*std::shared_ptr<Cookie> c(new Cookie(...));

  return c;*/
}
