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

//namespace cookie {

size_t CookieJar::size() const noexcept {
  return cookies_.size();
}

bool CookieJar::is_empty() const noexcept {
  return size() == 0;
}

/* If vector:
bool CookieJar::add(const Cookie& cookie) {
  cookies_.push_back(cookie);
  return true;
}*/

// If set:
bool CookieJar::add(const Cookie& cookie) {
  return cookies_.insert(cookie).second;
}

/*If vector:
bool CookieJar::add(const std::string& name, const std::string& value) {
  try {
    Cookie c{name, value};
    cookies_.push_back(c);
    return true;

  } catch(CookieException& ce) {
    return false;
  }
}*/

// If set:
bool CookieJar::add(const std::string& name, const std::string& value) {
  try {
    Cookie c{name, value};
    return cookies_.insert(c).second;

  } catch (CookieException& ce) {
    return false;
  }
}

/* If vector:
bool CookieJar::add(const std::string& name, const std::string& value, const std::vector<std::string>& options) {
  try {
    Cookie c{name, value, options};
    cookies_.push_back(c);
    return true;

  } catch (CookieException& ce) {
    return false;
  }
}*/

// If set:
bool CookieJar::add(const std::string& name, const std::string& value, const std::vector<std::string>& options) {
  try {
    Cookie c{name, value, options};
    return cookies_.insert(c).second;

  } catch (CookieException& ce) {
    return false;
  }
}

/*void CookieJar::remove(const Cookie& cookie) noexcept {
  cookies_.erase(cookie);
}*/

// If set:
CookieJar& CookieJar::remove(const std::string& name) noexcept {
  std::set<Cookie>::iterator result = std::find_if(cookies_.begin(), cookies_.end(), find_by_name(name));

  if(result not_eq cookies_.end())
    cookies_.erase(*result);

  return *this;
}

/* If vector:
CookieJar& CookieJar::remove(const std::string& name) noexcept {
  for(size_t i = 0; i < cookies_.size(); i++) {
    if(cookies_[i].get_name() == name)
      cookies_.erase(cookies_.begin() + i);
  }

  return *this;
}*/

// If set:
CookieJar& CookieJar::remove(const std::string& name, const std::string& value) noexcept {
  std::set<Cookie>::iterator result = std::find_if(cookies_.begin(), cookies_.end(), find_by_name_and_value(name, value));

  if(result not_eq cookies_.end())
    cookies_.erase(*result);

  return *this;
}

/* If vector:
CookieJar& CookieJar::remove(const std::string& name, const std::string& value) noexcept {
  for(size_t i = 0; i < cookies_.size(); i++) {
    if(cookies_[i].get_name() == name and cookies_[i].get_value() == value)
      cookies_.erase(cookies_.begin() + i);
  }

  return *this;
}*/

// If set:
CookieJar& CookieJar::clear() noexcept {
  cookies_.erase(cookies_.begin(), cookies_.end());
  return *this;
}

/* If vector:
CookieJar& CookieJar::clear() noexcept {
  cookies_.clear();
  return *this;
}*/

/*bool CookieJar::exists(const Cookie& cookie) noexcept {
  return cookies_.find(cookie) not_eq cookies_.end();
}*/

// If set:
bool CookieJar::exists(const std::string& name) noexcept {
  std::set<Cookie>::iterator result = std::find_if(cookies_.begin(), cookies_.end(), find_by_name(name));
  return (result not_eq cookies_.end());
}

/* If vector:
bool CookieJar::exists(const std::string& name) noexcept {
  for(size_t i = 0; i < cookies_.size(); i++) {
    if(cookies_[i].get_name() == name)
      return true;
  }

  return false;
}*/

// If set:
bool CookieJar::exists(const std::string& name, const std::string& value) noexcept {
  std::set<Cookie>::iterator result = std::find_if(cookies_.begin(), cookies_.end(), find_by_name_and_value(name, value));
  return (result not_eq cookies_.end());
}

/* If vector:
bool CookieJar::exists(const std::string& name, const std::string& value) noexcept {
  for(size_t i = 0; i < cookies_.size(); i++) {
    if(cookies_[i].get_name() == name and cookies_[i].get_value() == value)
      return true;
  }

  return false;
}*/

/*Cookie CookieJar::get_cookie(const Cookie& cookie) noexcept {
  auto it = cookies_.find(cookie);
  return (it not_eq cookies_.end()) ? *it : *cookies_.insert(cookie).first;
}*/

// If set:
Cookie CookieJar::get_cookie(const std::string& name) { // noexcept
  std::set<Cookie>::iterator result = std::find_if(cookies_.begin(), cookies_.end(), find_by_name(name));
  return (result not_eq cookies_.end()) ? *result : throw CookieException{"Cookie not found!"}; // *cookies_.insert(c).first;
}

/* If vector:
Cookie CookieJar::get_cookie(const std::string& name) { // noexcept
  for(size_t i = 0; i < cookies_.size(); i++) {
    if(cookies_[i].get_name() == name)
      return cookies_[i];
  }

  // TODO: Better solution than throw exception?
  throw CookieException{"Cookie not found!"};
}*/

// If set:
Cookie CookieJar::get_cookie(const std::string& name, const std::string& value) { // noexcept
  std::set<Cookie>::iterator result = std::find_if(cookies_.begin(), cookies_.end(), find_by_name_and_value(name, value));
  return (result not_eq cookies_.end()) ? *result : throw CookieException{"Cookie not found!"}; // *cookies_.insert(c).first;
}

/* If vector:
Cookie CookieJar::get_cookie(const std::string& name, const std::string& value) { // noexcept
  for(size_t i = 0; i < cookies_.size(); i++) {
    if(cookies_[i].get_name() == name and cookies_[i].get_value() == value)
      return cookies_[i];
  }

  // TODO: Better solution than throw exception?
  throw CookieException{"Cookie not found!"};
}*/

// If set:
std::vector<Cookie> CookieJar::get_cookies() const {
  return std::vector<Cookie>{cookies_.begin(), cookies_.end()};
}

/* If vector:
std::vector<Cookie> CookieJar::get_cookies() const {
  return cookies_;
}*/

// If set:

std::set<Cookie>::const_iterator CookieJar::begin() const noexcept {
  return cookies_.cbegin();
}

std::set<Cookie>::const_iterator CookieJar::end() const noexcept {
  return cookies_.cend();
}

//};  // < namespace cookie
