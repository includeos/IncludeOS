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

#include <map>
#include <string>

#include <net/http/cookie.hpp>
#include <mana/attribute.hpp>

namespace cookie {

class CookieJar : public mana::Attribute {
public:

  explicit CookieJar() = default;

  CookieJar(const CookieJar&) = default;

  CookieJar(CookieJar&&) = default;

  CookieJar& operator = (CookieJar&&) = default;

  ~CookieJar() = default;

  size_t size() const noexcept;

  bool empty() const noexcept;

  bool insert(const http::Cookie& c) noexcept;

  bool insert(const std::string& name, const std::string& value = "");

  CookieJar& erase(const http::Cookie& c) noexcept;

  CookieJar& erase(const std::string& name) noexcept;

  CookieJar& clear() noexcept;

  bool exists(const std::string& name) const noexcept;

  const std::string& cookie_value(const std::string& name) const noexcept;

  const std::map<std::string, std::string>& get_cookies() const noexcept;

  std::map<std::string, std::string>::const_iterator begin() const noexcept;

  std::map<std::string, std::string>::const_iterator end() const noexcept;

private:
  std::map<std::string, std::string> cookies_;

  CookieJar& operator = (const CookieJar&) = delete;
}; //< class CookieJar

}; //< namespace cookie

#endif //< COOKIE_JAR_HPP
