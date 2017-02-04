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

#include "../../mod/http-parser/http_parser.h"
#include <ostream>

#include <uri>

namespace uri {

///////////////////////////////////////////////////////////////////////////////
inline static uint16_t bind_port(const std::experimental::string_view scheme,
                                 const uint16_t port_from_uri) noexcept
{
  const static std::unordered_map<std::experimental::string_view, uint16_t> port_table
  {
    {"ftp",    21U},
    {"http",   80U},
    {"https",  443U},
    {"irc",    6667U},
    {"ldap",   389U},
    {"nntp",   119U},
    {"rtsp",   554U},
    {"sip",    5060U},
    {"sips",   5061U},
    {"smtp",   25U},
    {"ssh",    22U},
    {"telnet", 23U},
    {"ws",     80U},
    {"xmpp",   5222U}
  };

  if (port_from_uri not_eq 0) return port_from_uri;

  const auto it = port_table.find(scheme);

  return (it not_eq port_table.cend()) ? it->second : 0xFFFFU;
}

///////////////////////////////////////////////////////////////////////////////
// copy helper
static inline std::experimental::string_view updated_copy(
  const std::string& to_copy,
  const std::experimental::string_view& view,
  const std::string& from_copy)
{
  const auto offs = view.data() - from_copy.data();
  return {to_copy.data() + offs, view.size()};
}

///////////////////////////////////////////////////////////////////////////////
URI::URI(const char* uri, const bool parse)
  : uri_str_{decode(uri)}
{
  if (parse) this->parse();
}

///////////////////////////////////////////////////////////////////////////////
URI::URI(const std::experimental::string_view uri, const bool parse)
  : uri_str_{decode(uri)}
{
  if (parse) this->parse();
}

///////////////////////////////////////////////////////////////////////////////
URI::URI(const URI& u)
  : uri_str_{u.uri_str_},
    port_{u.port_},
    scheme_{updated_copy(uri_str_, u.scheme_, u.uri_str_)},
    userinfo_{updated_copy(uri_str_, u.userinfo_, u.uri_str_)},
    host_{updated_copy(uri_str_, u.host_, u.uri_str_)},
    port_str_{updated_copy(uri_str_, u.port_str_, u.uri_str_)},
    path_{updated_copy(uri_str_, u.path_, u.uri_str_)},
    query_{updated_copy(uri_str_, u.query_, u.uri_str_)},
    fragment_{updated_copy(uri_str_, u.fragment_, u.uri_str_)},
    query_map_{}
{
  for(const auto& ent : u.query_map_)
  {
    query_map_.emplace(
      updated_copy(uri_str_, ent.first, u.uri_str_),
      updated_copy(uri_str_, ent.second, u.uri_str_)
    );
  }
}

///////////////////////////////////////////////////////////////////////////////
URI::URI(URI&& u)
  : uri_str_(std::move(u.uri_str_)),
    port_(u.port_),
    scheme_(u.scheme_),
    userinfo_(u.userinfo_),
    host_(u.host_),
    port_str_(u.port_str_),
    path_(u.path_),
    query_(u.query_),
    fragment_(u.fragment_),
    query_map_(std::move(u.query_map_))
{
}

///////////////////////////////////////////////////////////////////////////////
URI& URI::operator=(const URI& u)
{
  uri_str_  = u.uri_str_;
  port_     = u.port_;
  scheme_   = updated_copy(uri_str_, u.scheme_, u.uri_str_);
  userinfo_ = updated_copy(uri_str_, u.userinfo_, u.uri_str_);
  host_     = updated_copy(uri_str_, u.host_, u.uri_str_);
  port_str_ = updated_copy(uri_str_, u.port_str_, u.uri_str_);
  path_     = updated_copy(uri_str_, u.path_, u.uri_str_);
  query_    = updated_copy(uri_str_, u.query_, u.uri_str_);
  fragment_ = updated_copy(uri_str_, u.fragment_, u.uri_str_);

  query_map_.clear();

  for(const auto& ent : u.query_map_)
  {
    query_map_.emplace(
      updated_copy(uri_str_, ent.first, u.uri_str_),
      updated_copy(uri_str_, ent.second, u.uri_str_)
    );
  }

  return *this;
}

///////////////////////////////////////////////////////////////////////////////
URI& URI::operator=(URI&& u)
{
  uri_str_  = std::move(u.uri_str_);
  port_     = u.port_;
  scheme_   = u.scheme_;
  userinfo_ = u.userinfo_;
  host_     = u.host_;
  port_str_ = u.port_str_;
  path_     = u.path_;
  query_    = u.query_;
  fragment_ = u.fragment_;
  query_map_ = std::move(u.query_map_);
  return *this;
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
bool URI::host_is_ip4() const noexcept {
  return std::isdigit(host_.back());
}

///////////////////////////////////////////////////////////////////////////////
std::experimental::string_view URI::host_and_port() const noexcept {
  if (not port_str_.empty()) {
    return std::experimental::string_view{host_.data(), host_.length() + port_str_.length() + 1};
  } else {
    return host_;
  }
}

///////////////////////////////////////////////////////////////////////////////
std::experimental::string_view URI::port_str() const noexcept {
  return port_str_;
}

///////////////////////////////////////////////////////////////////////////////
uint16_t URI::port() const noexcept {
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
std::experimental::string_view URI::query(const std::experimental::string_view key) {
  if (query_map_.empty() and (not query_.empty())) {
    load_queries();
  }

  const auto target = query_map_.find(key);

  return (target not_eq query_map_.cend()) ? target->second : std::experimental::string_view{};
}

///////////////////////////////////////////////////////////////////////////////
bool URI::is_valid() const noexcept {
  return (not host_.empty()) or (not path_.empty()) ;
}

///////////////////////////////////////////////////////////////////////////////
URI::operator bool() const noexcept {
  return is_valid();
}

///////////////////////////////////////////////////////////////////////////////
std::experimental::string_view URI::to_string() const noexcept {
  return uri_str_;
}

const std::string& URI::str() const noexcept {
  return uri_str_;
}

///////////////////////////////////////////////////////////////////////////////
URI::operator std::string () const {
  return uri_str_;
}

///////////////////////////////////////////////////////////////////////////////
URI& URI::operator << (const std::string& chunk) {
  uri_str_.append(chunk);
  return *this;
}

///////////////////////////////////////////////////////////////////////////////
URI& URI::parse() {
  http_parser_url u;
  http_parser_url_init(&u);

  const auto p = uri_str_.data();

  const int result = http_parser_parse_url(p, uri_str_.length(), 0, &u);

#ifdef URI_THROW_ON_ERROR
  if (result not_eq 0) {
    throw URI_error{"Invalid uri: " + uri_str_};
  }
#endif //< URI_THROW_ON_ERROR

  (void)result;

  using sview = std::experimental::string_view;

  scheme_   = (u.field_set & (1 << UF_SCHEMA))   ? sview{p + u.field_data[UF_SCHEMA].off,   u.field_data[UF_SCHEMA].len}   : sview{};
  userinfo_ = (u.field_set & (1 << UF_USERINFO)) ? sview{p + u.field_data[UF_USERINFO].off, u.field_data[UF_USERINFO].len} : sview{};
  host_     = (u.field_set & (1 << UF_HOST))     ? sview{p + u.field_data[UF_HOST].off,     u.field_data[UF_HOST].len}     : sview{};
  port_str_ = (u.field_set & (1 << UF_PORT))     ? sview{p + u.field_data[UF_PORT].off,     u.field_data[UF_PORT].len}     : sview{};
  path_     = (u.field_set & (1 << UF_PATH))     ? sview{p + u.field_data[UF_PATH].off,     u.field_data[UF_PATH].len}     : sview{};
  query_    = (u.field_set & (1 << UF_QUERY))    ? sview{p + u.field_data[UF_QUERY].off,    u.field_data[UF_QUERY].len}    : sview{};
  fragment_ = (u.field_set & (1 << UF_FRAGMENT)) ? sview{p + u.field_data[UF_FRAGMENT].off, u.field_data[UF_FRAGMENT].len} : sview{};

  port_ = bind_port(scheme_, u.port);

  return *this;
}

///////////////////////////////////////////////////////////////////////////////
URI& URI::reset() {
  new (this) URI{};
  return *this;
}

/////////////////////////////////////////////////////////////////////////////
void URI::load_queries() {
  auto _ = query_;

  std::experimental::string_view name  {};
  std::experimental::string_view value {};
  std::experimental::string_view::size_type base {0U};
  std::experimental::string_view::size_type break_point {};

  _.remove_prefix(_.find_first_not_of(' '));

  while (true) {
    if ((break_point = _.find('=')) not_eq std::experimental::string_view::npos) {
      name = _.substr(base, break_point);
      //-----------------------------------
      _.remove_prefix(name.length() + 1U);
    }
    else {
      break;
    }

    if ((break_point = _.find('&')) not_eq std::experimental::string_view::npos) {
      value = _.substr(base, break_point);
      query_map_.emplace(name, value);
      _.remove_prefix(value.length() + 1U);
    }
    else {
      query_map_.emplace(name, _);
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
