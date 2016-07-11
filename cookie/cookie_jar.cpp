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

#include "cookie_jar.hpp"

namespace cookie {

size_t CookieJar::size() const noexcept {
  return cookies_.size();
}

bool CookieJar::is_empty() const noexcept {
  return size() == 0;
}

/*bool CookieJar::add(const Cookie& cookie) {
  cookies_.push_back(cookie);
  return true;
}*/

bool CookieJar::add(std::string& name, std::string& value) {
  try {
    Cookie c{name, value};
    cookies_.push_back(c);
    return true;

  } catch(CookieException& ce) {
    return false;
  }
}

bool CookieJar::add(std::string& name, std::string& value, std::vector<std::string>& options) {
  try {
    Cookie c{name, value, options};
    cookies_.push_back(c);
    return true;

  } catch (CookieException& ce) {
    return false;
  }
}

/*void CookieJar::remove(const Cookie& cookie) noexcept {
  cookies_.erase(cookie);
}*/

void CookieJar::remove(const std::string& name) noexcept {
  for(size_t i = 0; i < cookies_.size(); i++) {
    if(cookies_[i].get_name() == name)
      cookies_.erase(cookies_.begin() + i);
  }
}

void CookieJar::remove(const std::string& name, const std::string& value) noexcept {
  for(size_t i = 0; i < cookies_.size(); i++) {
    if(cookies_[i].get_name() == name and cookies_[i].get_value() == value)
      cookies_.erase(cookies_.begin() + i);
  }
}

void CookieJar::clear() noexcept {
  cookies_.clear();
}

/*bool CookieJar::exists(const Cookie& cookie) noexcept {

}*/

bool CookieJar::exists(const std::string& name) noexcept {
  for(size_t i = 0; i < cookies_.size(); i++) {
    if(cookies_[i].get_name() == name)
      return true;
  }

  return false;
}

bool CookieJar::exists(const std::string& name, const std::string& value) noexcept {
  for(size_t i = 0; i < cookies_.size(); i++) {
    if(cookies_[i].get_name() == name and cookies_[i].get_value() == value)
      return true;
  }

  return false;
}

/*Cookie CookieJar::get_cookie(const Cookie& cookie) noexcept {

}*/

Cookie CookieJar::get_cookie(const std::string& name) { // noexcept
  for(size_t i = 0; i < cookies_.size(); i++) {
    if(cookies_[i].get_name() == name)
      return cookies_[i];
  }

  // TODO: Better solution than throw exception?
  throw CookieException{"Cookie not found!"};
}

Cookie CookieJar::get_cookie(const std::string& name, const std::string& value) { // noexcept
  for(size_t i = 0; i < cookies_.size(); i++) {
    if(cookies_[i].get_name() == name and cookies_[i].get_value() == value)
      return cookies_[i];
  }

  // TODO: Better solution than throw exception?
  throw CookieException{"Cookie not found!"};
}

std::vector<Cookie> CookieJar::get_cookies() const {
  return cookies_;
}

};  // < namespace cookie
