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

#include <uri>
#include <regex>
#include <ostream>

namespace uri {

///////////////////////////////////////////////////////////////////////////////
URI::URI(const std::experimental::string_view uri)
  : uri_str_{decode(std::move(uri.to_string()))}
{
  parse();
}

///////////////////////////////////////////////////////////////////////////////
std::experimental::string_view URI::scheme() const noexcept {
  return scheme_;
}

///////////////////////////////////////////////////////////////////////////////
std::experimental::string_view URI::userinfo() const noexcept {
  return userinfo_;
}

///////////////////////////////////////////////////////////////////////////////
std::experimental::string_view URI::host() const noexcept {
  return host_;
}

///////////////////////////////////////////////////////////////////////////////
std::experimental::string_view URI::port_str() const noexcept {
  return port_str_;
}

///////////////////////////////////////////////////////////////////////////////
uint16_t URI::port() const noexcept {
  if ((port_ == -1) && (not port_str_.empty())) {
    port_ = static_cast<int32_t>(std::stoi(port_str_.to_string()));
  }
  return port_;
}

///////////////////////////////////////////////////////////////////////////////
std::experimental::string_view URI::path() const noexcept {
  return path_;
}

///////////////////////////////////////////////////////////////////////////////
std::experimental::string_view URI::query() const noexcept {
  return query_;
}

///////////////////////////////////////////////////////////////////////////////
std::experimental::string_view URI::fragment() const noexcept {
  return fragment_;
}

///////////////////////////////////////////////////////////////////////////////
const std::string& URI::query(const std::string& key) {
  static const std::string no_entry_value;

  if (not query_.empty() and queries_.empty()) {
    load_queries();
  }

  auto target = queries_.find(key);

  return (target not_eq queries_.end()) ? target->second : no_entry_value;
}

///////////////////////////////////////////////////////////////////////////////
bool URI::is_valid() const noexcept {
  return not host_.empty() or not path_.empty();
}

///////////////////////////////////////////////////////////////////////////////
URI::operator bool() const noexcept {
  return is_valid();
}

///////////////////////////////////////////////////////////////////////////////
std::string URI::to_string() const {
  return uri_str_;
}

///////////////////////////////////////////////////////////////////////////////
URI::operator std::string () const {
  return uri_str_;
}

///////////////////////////////////////////////////////////////////////////////
void URI::parse() {
  using sview = std::experimental::string_view;

  static const std::regex uri_pattern
  {
    "^([a-zA-Z]+[\\w\\+\\-\\.]+)?(\\://)?" //< scheme
    "(([^:@]+)(\\:([^@]+))?@)?"            //< username && password
    "([^/:?#]+)?(\\:(\\d+))?"              //< hostname && port
    "([^?#]+)"                             //< path
    "(\\?([^#]*))?"                        //< query
    "(#(.*))?$"                            //< fragment
  };

  static std::smatch uri_parts;

  if (std::regex_match(uri_str_, uri_parts, uri_pattern)) {
    path_     = sview(uri_str_.data() + uri_parts.position(10), uri_parts.length(10));

    scheme_   = uri_parts.length(1)  ? sview(uri_str_.data() + uri_parts.position(1),  uri_parts.length(1))  : sview{};
    userinfo_ = uri_parts.length(3)  ? sview(uri_str_.data() + uri_parts.position(3),  uri_parts.length(3))  : sview{};
    host_     = uri_parts.length(7)  ? sview(uri_str_.data() + uri_parts.position(7),  uri_parts.length(7))  : sview{};
    port_str_ = uri_parts.length(9)  ? sview(uri_str_.data() + uri_parts.position(9),  uri_parts.length(9))  : sview{};
    query_    = uri_parts.length(11) ? sview(uri_str_.data() + uri_parts.position(11), uri_parts.length(11)) : sview{};
    fragment_ = uri_parts.length(13) ? sview(uri_str_.data() + uri_parts.position(13), uri_parts.length(13)) : sview{};
  }
}

/////////////////////////////////////////////////////////////////////////////
void URI::load_queries() {
  static const std::regex query_token_pattern {"[^?=&]+"};

  auto query = query_.to_string();

  auto it  = std::sregex_iterator(query.cbegin(), query.cend(), query_token_pattern);
  auto end = std::sregex_iterator();

  while (it not_eq end) {
    auto key = it->str();
    if (++it not_eq end) {
      queries_[key] = it->str();
    } else {
      queries_[key];
    }
  } 
}

///////////////////////////////////////////////////////////////////////////////
bool operator < (const URI& lhs, const URI& rhs) noexcept {
  return lhs.to_string() < rhs.to_string();
}

///////////////////////////////////////////////////////////////////////////////
bool operator == (const URI& lhs, const URI& rhs) noexcept {
  return lhs.to_string() == rhs.to_string();
}

///////////////////////////////////////////////////////////////////////////////
std::ostream& operator<< (std::ostream& output_device, const URI& uri) {
  return output_device << uri.to_string();
}

} //< namespace uri
