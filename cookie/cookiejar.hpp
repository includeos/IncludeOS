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

#ifndef COOKIE_JAR_HPP
#define COOKIE_JAR_HPP

#include <vector>
#include <string>

// <set> not supported

#include "cookie.hpp"

using namespace cookie;

class CookieJar {
public:

  explicit CookieJar() = default;

  ~CookieJar();

  bool add(std::string& name, std::string& value);

  bool add(std::string& name, std::string& value, std::vector<std::string>& options);

  Cookie find(const std::string& name);

  Cookie find(const std::string& name, const std::string& value);

private:
  std::vector<Cookie> cookies_;

  struct find_by_name {
    find_by_name(const std::string& name) : name_{name} {}

    bool operator()(const Cookie& c) {
      return c.get_name() == name_;
    }

  private:
    std::string name_;
  };
};

/**--v----------- Implementation Details -----------v--**/

CookieJar::~CookieJar() {

}

inline bool CookieJar::add(std::string& name, std::string& value) {
  try {
    Cookie c{name, value};
    cookies_.push_back(c);
    return true;

  } catch(CookieException& ce) {
    return false;
  }
}

inline bool CookieJar::add(std::string& name, std::string& value, std::vector<std::string>& options) {
  try {
    Cookie c{name, value, options};
    cookies_.push_back(c);
    return true;

  } catch (CookieException& ce) {
    return false;
  }
}

inline Cookie CookieJar::find(const std::string& name) {

  for(size_t i = 0; i < cookies_.size(); i++) {
    if(cookies_[i].get_name() == name)
      return cookies_[i];
  }

  // TODO: Better solution than throw exception?
  throw CookieException("Cookie not found!");

  /* set:
  std::set<Cookie>::iterator it = std::find_if(cookies_.begin(),
    cookies_.end(), find_by_name(name));

  if(it not_eq cookies_.end())
    return *it;

  return *cookies_.emplace(name, value).first;*/
}

inline Cookie CookieJar::find(const std::string& name, const std::string& value) {

  for(size_t i = 0; i < cookies_.size(); i++) {
    if(cookies_[i].get_name() == name and cookies_[i].get_value() == value)
      return cookies_[i];
  }

  // TODO: Better solution than throw exception?
  throw CookieException("Cookie not found!");

  /*set: auto it = cookies_.find(Cookie{name, value});

  if(it not_eq cookies_.end())
    return *it;

  return *cookies_.emplace(name, value).first;*/
}

#endif  // < COOKIE_JAR_HPP
