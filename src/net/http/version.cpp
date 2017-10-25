// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2017 Oslo and Akershus University College of Applied Sciences
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

#include <cstdlib>
#include <net/http/version.hpp>

namespace http {

///////////////////////////////////////////////////////////////////////////////
Version::Version(const unsigned major, const unsigned minor) noexcept:
  major_{major},
  minor_{minor}
{}

///////////////////////////////////////////////////////////////////////////////
unsigned Version::major() const noexcept {
  return major_;
}

///////////////////////////////////////////////////////////////////////////////
void Version::set_major(const unsigned major) noexcept {
  major_ = major;
}

///////////////////////////////////////////////////////////////////////////////
unsigned Version::minor() const noexcept {
  return minor_;
}

///////////////////////////////////////////////////////////////////////////////
void Version::set_minor(const unsigned minor) noexcept {
  minor_ = minor;
}

///////////////////////////////////////////////////////////////////////////////
std::string Version::to_string() const {
  char http_ver[12];
  snprintf(http_ver, sizeof(http_ver), "%s%u.%u", "HTTP/", major_, minor_);
  return http_ver;
}

///////////////////////////////////////////////////////////////////////////////
Version::operator std::string() const {
  return to_string();
}

///////////////////////////////////////////////////////////////////////////////
bool operator==(const Version& lhs, const Version& rhs) noexcept {
  return lhs.major() == rhs.major()
         and
         lhs.minor() == rhs.minor();
}

///////////////////////////////////////////////////////////////////////////////
bool operator!=(const Version& lhs, const Version& rhs) noexcept {
  return not (lhs == rhs);
}

///////////////////////////////////////////////////////////////////////////////
bool operator<(const Version& lhs, const Version& rhs) noexcept {
  return (lhs.major() == rhs.major()) ? (lhs.minor() < rhs.minor()) : (lhs.major() < rhs.major());
}

///////////////////////////////////////////////////////////////////////////////
bool operator>(const Version& lhs, const Version& rhs) noexcept {
  return (lhs.major() == rhs.major()) ? (lhs.minor() > rhs.minor()) : (lhs.major() > rhs.major());
}

///////////////////////////////////////////////////////////////////////////////
bool operator<=(const Version& lhs, const Version& rhs) noexcept {
  return (lhs < rhs) or (lhs == rhs);
}

///////////////////////////////////////////////////////////////////////////////
bool operator>=(const Version& lhs, const Version& rhs) noexcept {
  return (lhs > rhs) or (lhs == rhs);
}

} //< namespace http
