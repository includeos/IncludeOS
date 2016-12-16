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

#include <mana/attributes/cookie_jar.hpp>

namespace cookie {

///////////////////////////////////////////////////////////////////////////////
size_t CookieJar::size() const noexcept {
  return cookies_.size();
}

///////////////////////////////////////////////////////////////////////////////
bool CookieJar::empty() const noexcept {
  return cookies_.empty();
}

///////////////////////////////////////////////////////////////////////////////
bool CookieJar::insert(const http::Cookie& c) noexcept {
  return cookies_.emplace(std::make_pair(c.get_name(), c.get_value())).second;
}

///////////////////////////////////////////////////////////////////////////////
bool CookieJar::insert(const std::string& name, const std::string& value) {
  return cookies_.emplace(std::make_pair(name, value)).second;
}

///////////////////////////////////////////////////////////////////////////////
CookieJar& CookieJar::erase(const http::Cookie& c) noexcept {
  cookies_.erase(c.get_name());
  return *this;
}

///////////////////////////////////////////////////////////////////////////////
CookieJar& CookieJar::erase(const std::string& name) noexcept {
  cookies_.erase(name);
  return *this;
}

///////////////////////////////////////////////////////////////////////////////
CookieJar& CookieJar::clear() noexcept {
  cookies_.erase(cookies_.begin(), cookies_.end());
  return *this;
}

///////////////////////////////////////////////////////////////////////////////
bool CookieJar::exists(const std::string& name) const noexcept {
  return cookies_.find(name) not_eq cookies_.end();
}

///////////////////////////////////////////////////////////////////////////////
const std::string& CookieJar::cookie_value(const std::string& name) const noexcept {
  static const std::string no_entry_value;

  auto it = cookies_.find(name);

  if (it not_eq cookies_.end()) {
    return it->second;
  }

  return no_entry_value;
}

///////////////////////////////////////////////////////////////////////////////
const std::map<std::string, std::string>& CookieJar::get_cookies() const noexcept {
  return cookies_;
}

///////////////////////////////////////////////////////////////////////////////
std::map<std::string, std::string>::const_iterator CookieJar::begin() const noexcept {
  return cookies_.cbegin();
}

///////////////////////////////////////////////////////////////////////////////
std::map<std::string, std::string>::const_iterator CookieJar::end() const noexcept {
  return cookies_.cend();
}

}; //< namespace cookie
