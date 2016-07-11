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

namespace cookie {

class CookieJar {
public:

  explicit CookieJar() = default;

  // CookieJar(CookieJar&&) = default;

  // CookieJar& operator = (CookieJar&&) = default;

  ~CookieJar() = default;

  size_t size() const noexcept;

  bool is_empty() const noexcept;

  // bool add(const Cookie& cookie);

  bool add(std::string& name, std::string& value);
  // instead of bool add(const Cookie& cookie) because we want the developer to be able to add to the CookieJar in service.cpp
  // without having to create Cookies first

  bool add(std::string& name, std::string& value, std::vector<std::string>& options);

  // void remove(const Cookie& cookie) noexcept;

  void remove(const std::string& name) noexcept;

  void remove(const std::string& name, const std::string& value) noexcept;

  void clear() noexcept;

  // bool exists(const Cookie& cookie) noexcept;

  bool exists(const std::string& name) noexcept;

  bool exists(const std::string& name, const std::string& value) noexcept;

  // Cookie get_cookie(const Cookie& cookie) noexcept;

  Cookie get_cookie(const std::string& name); // noexcept

  Cookie get_cookie(const std::string& name, const std::string& value); // noexcept

  std::vector<Cookie> get_cookies() const;

private:
  std::vector<Cookie> cookies_;

  // CookieJar(const CookieJar&) = delete;

  // CookieJar& operator = (const CookieJar&) = delete;

  /*struct find_by_name {
    find_by_name(const std::string& name) : name_{name} {}

    bool operator()(const Cookie& c) {
      return c.get_name() == name_;
    }

  private:
    std::string name_;
  };  // < struct find_by_name */

};  // < class CookieJar

};  // < namespace cookie

#endif  // < COOKIE_JAR_HPP
