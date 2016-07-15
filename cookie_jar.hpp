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
#include <set>

#include "cookie.hpp"

#include "attribute.hpp"

using namespace cookie;

//namespace cookie {

class CookieJar : public server::Attribute {
public:

  // If set:

  struct find_by_name {
    find_by_name(const std::string& name) : name_{name} {}

    bool operator ()(const Cookie& c) {
      return c.get_name() == name_;
    }

  private:
    std::string name_;
  };  // < struct find_by_name

  struct find_by_name_and_value {
    find_by_name_and_value(const std::string& name, const std::string& value) : name_{name}, value_{value} {}

    bool operator ()(const Cookie& c) {
      return c.get_name() == name_ && c.get_value() == value_;
    }

  private:
    std::string name_;
    std::string value_;
  };  // < struct find_by_name_and_value

  explicit CookieJar() = default;

  // Because we want to send a CookieJar to CookieParser
  CookieJar(const CookieJar&) = default;

  CookieJar(CookieJar&&) = default;

  CookieJar& operator = (CookieJar&&) = default;

  ~CookieJar() = default;

  size_t size() const noexcept;

  bool is_empty() const noexcept;

  bool add(const Cookie& cookie);

  bool add(const std::string& name, const std::string& value);
  // instead of bool add(const Cookie& cookie) because we want the developer to be able to add to the CookieJar in service.cpp
  // without having to create Cookies first

  bool add(const std::string& name, const std::string& value, const std::vector<std::string>& options);

  // bool update(const Cookie& old_cookie, const Cookie& new_cookie);

  // void remove(const Cookie& cookie) noexcept;

  CookieJar& remove(const std::string& name) noexcept;

  CookieJar& remove(const std::string& name, const std::string& value) noexcept;

  CookieJar& clear() noexcept;

  // bool exists(const Cookie& cookie) noexcept;

  bool exists(const std::string& name) noexcept;

  bool exists(const std::string& name, const std::string& value) noexcept;

  // Cookie get_cookie(const Cookie& cookie) noexcept;

  Cookie get_cookie(const std::string& name); // noexcept

  Cookie get_cookie(const std::string& name, const std::string& value); // noexcept

  std::vector<Cookie> get_cookies() const;

  // If set:

  std::set<Cookie>::const_iterator begin() const noexcept;

  std::set<Cookie>::const_iterator end() const noexcept;

private:
  //std::vector<Cookie> cookies_;
  std::set<Cookie> cookies_;

  // Not this because we want to send CookieJar to CookieParser: CookieJar(const CookieJar&) = delete;

  CookieJar& operator = (const CookieJar&) = delete;

};  // < class CookieJar

//};  // < namespace cookie

#endif  // < COOKIE_JAR_HPP
