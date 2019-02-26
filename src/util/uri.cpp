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

#include <algorithm>
#include <cassert>
#include <cctype>
#include <ostream>
#include <uri>
#include <utility>
#include <vector>
#include <array>

#include <http-parser/http_parser.h>

namespace uri {

static inline std::vector<char> decoded_vector(util::csview input)
{
  auto res = decode(input);
  return {res.begin(), res.end()};
}

///////////////////////////////////////////////////////////////////////////////
static inline bool icase_equal(util::csview lhs, util::csview rhs) noexcept {
  return (lhs.size() == rhs.size())
         and
         std::equal(lhs.cbegin(), lhs.cend(), rhs.cbegin(), [](const char a, const char b) {
          return std::tolower(a) == std::tolower(b);
         });
}

///////////////////////////////////////////////////////////////////////////////
static inline uint16_t bind_port(util::csview scheme, const uint16_t port_from_uri) noexcept {
  static const std::vector<std::pair<util::csview, uint16_t>> port_table
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
    {"wss",    443U},
    {"xmpp",   5222U},
  };

  if (port_from_uri not_eq 0) return port_from_uri;

  const auto it = std::find_if(port_table.cbegin(), port_table.cend(), [scheme](const auto& _) {
      return icase_equal(_.first, scheme);
  });

  return (it not_eq port_table.cend()) ? it->second : 0xFFFFU;
}

///////////////////////////////////////////////////////////////////////////////
// copy helper
///////////////////////////////////////////////////////////////////////////////
static inline util::sview updated_copy(const std::vector<char>& to_copy,
                                       util::csview view,
                                       const std::vector<char>& from_copy)
{
  // sometimes the source is empty, but we need a valid empty string
  if (view.data() == nullptr) return {&to_copy.back(), 0};
  return {&to_copy.data()[view.data() - from_copy.data()], view.size()};
}

///////////////////////////////////////////////////////////////////////////////
URI::URI(const char* uri, const bool parse)
  : uri_str_{decoded_vector(uri)}
{
  if (parse) this->parse();
}

///////////////////////////////////////////////////////////////////////////////
URI::URI(const char* uri, const size_t count, const bool parse)
  : uri_str_{decoded_vector(util::csview{uri, count})}
{
  if (parse) this->parse();
}

///////////////////////////////////////////////////////////////////////////////
URI::URI(const std::string& uri, const bool parse)
  : uri_str_{decoded_vector(util::csview{uri.data(), uri.length()})}
{
  if (parse) this->parse();
}

///////////////////////////////////////////////////////////////////////////////
URI::URI(util::csview uri, const bool parse)
  : uri_str_{decoded_vector(uri)}
{
  if (parse) this->parse();
}

///////////////////////////////////////////////////////////////////////////////
URI::URI(const URI& u)
  : uri_str_  {u.uri_str_}
  , port_     {u.port_}
  , scheme_   {updated_copy(uri_str_, u.scheme_,   u.uri_str_)}
  , userinfo_ {updated_copy(uri_str_, u.userinfo_, u.uri_str_)}
  , host_     {updated_copy(uri_str_, u.host_,     u.uri_str_)}
  , path_     {updated_copy(uri_str_, u.path_,     u.uri_str_)}
  , query_    {updated_copy(uri_str_, u.query_,    u.uri_str_)}
  , fragment_ {updated_copy(uri_str_, u.fragment_, u.uri_str_)}
  , query_map_{}
{
  for(const auto& ent : u.query_map_)
  {
    query_map_.emplace(updated_copy(uri_str_, ent.first,  u.uri_str_),
                       updated_copy(uri_str_, ent.second, u.uri_str_));
  }
}

///////////////////////////////////////////////////////////////////////////////
URI::URI(URI&& u) noexcept
  : uri_str_{std::move(u.uri_str_)}
  , port_     {u.port_}
  , scheme_   {u.scheme_}
  , userinfo_ {u.userinfo_}
  , host_     {u.host_}
  , path_     {u.path_}
  , query_    {u.query_}
  , fragment_ {u.fragment_}
  , query_map_{std::move(u.query_map_)}
{
}

///////////////////////////////////////////////////////////////////////////////
URI& URI::operator=(const URI& u) {
  uri_str_  = u.uri_str_;
  port_     = u.port_;
  scheme_   = updated_copy(uri_str_, u.scheme_,   u.uri_str_);
  userinfo_ = updated_copy(uri_str_, u.userinfo_, u.uri_str_);
  host_     = updated_copy(uri_str_, u.host_,     u.uri_str_);
  path_     = updated_copy(uri_str_, u.path_,     u.uri_str_);
  query_    = updated_copy(uri_str_, u.query_,    u.uri_str_);
  fragment_ = updated_copy(uri_str_, u.fragment_, u.uri_str_);

  query_map_.clear();

  for(const auto& ent : u.query_map_) {
    query_map_.emplace(updated_copy(uri_str_, ent.first,  u.uri_str_),
                       updated_copy(uri_str_, ent.second, u.uri_str_));
  }

  return *this;
}

///////////////////////////////////////////////////////////////////////////////
URI& URI::operator=(URI&& u) noexcept {
  uri_str_   = std::move(u.uri_str_);
  port_      = u.port_;
  scheme_    = u.scheme_;
  userinfo_  = u.userinfo_;
  host_      = u.host_;
  path_      = u.path_;
  query_     = u.query_;
  fragment_  = u.fragment_;
  query_map_ = std::move(u.query_map_);

  return *this;
}

///////////////////////////////////////////////////////////////////////////////
util::sview URI::scheme() const noexcept {
  return scheme_;
}

bool URI::scheme_is_secure() const noexcept {
  return scheme() == "https" or scheme() == "wss";
}

///////////////////////////////////////////////////////////////////////////////
util::sview URI::userinfo() const noexcept {
  return userinfo_;
}

///////////////////////////////////////////////////////////////////////////////
util::sview URI::host() const noexcept {
  return host_;
}

///////////////////////////////////////////////////////////////////////////////
bool URI::host_is_ip4() const noexcept {
  return host_.empty() ? false : std::isdigit(host_.back());
}

///////////////////////////////////////////////////////////////////////////////
bool URI::host_is_ip6() const noexcept {
  return host_.empty() ? false : (*(host_.data() + host_.length()) == ']');
}

///////////////////////////////////////////////////////////////////////////////
std::string URI::host_and_port() const {
  return std::string{host_.data(), host_.length()} + ':' + std::to_string(port_);
}

///////////////////////////////////////////////////////////////////////////////
uint16_t URI::port() const noexcept {
  return port_;
}

///////////////////////////////////////////////////////////////////////////////
util::sview URI::path() const noexcept {
  return path_;
}

///////////////////////////////////////////////////////////////////////////////
util::sview URI::query() const noexcept {
  return query_;
}

///////////////////////////////////////////////////////////////////////////////
util::sview URI::fragment() const noexcept {
  return fragment_;
}

///////////////////////////////////////////////////////////////////////////////
util::sview URI::query(util::csview key) {
  if (query_map_.empty() and (not query_.empty())) {
    load_queries();
  }

  const auto target = query_map_.find(key);

  return (target not_eq query_map_.cend()) ? target->second : util::sview{};
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
std::string URI::to_string() const {
  return {uri_str_.begin(), uri_str_.end()};
}

///////////////////////////////////////////////////////////////////////////////
URI::operator std::string () const {
  return to_string();
}

///////////////////////////////////////////////////////////////////////////////
URI& URI::operator << (const std::string& chunk) {
  uri_str_.insert(uri_str_.end(), chunk.begin(), chunk.end());
  parse();
  return *this;
}

///////////////////////////////////////////////////////////////////////////////
URI& URI::parse() {
  http_parser_url u;
  http_parser_url_init(&u);

  const auto p = uri_str_.data();
  const auto result = http_parser_parse_url(p, uri_str_.size(), 0, &u);

#ifdef URI_THROW_ON_ERROR
  if (result not_eq 0) {
    std::string uri{uri_str_.begin(), uri_str_.end()};
    throw URI_error{"Invalid uri: " + uri};
  }
#endif //< URI_THROW_ON_ERROR

  (void)result;

  scheme_   = (u.field_set & (1 << UF_SCHEMA))   ? util::sview{p + u.field_data[UF_SCHEMA].off,   u.field_data[UF_SCHEMA].len}   : util::sview{};
  userinfo_ = (u.field_set & (1 << UF_USERINFO)) ? util::sview{p + u.field_data[UF_USERINFO].off, u.field_data[UF_USERINFO].len} : util::sview{};
  host_     = (u.field_set & (1 << UF_HOST))     ? util::sview{p + u.field_data[UF_HOST].off,     u.field_data[UF_HOST].len}     : util::sview{};
  path_     = (u.field_set & (1 << UF_PATH))     ? util::sview{p + u.field_data[UF_PATH].off,     u.field_data[UF_PATH].len}     : util::sview{};
  query_    = (u.field_set & (1 << UF_QUERY))    ? util::sview{p + u.field_data[UF_QUERY].off,    u.field_data[UF_QUERY].len}    : util::sview{};
  fragment_ = (u.field_set & (1 << UF_FRAGMENT)) ? util::sview{p + u.field_data[UF_FRAGMENT].off, u.field_data[UF_FRAGMENT].len} : util::sview{};

  auto port_str_ = (u.field_set & (1 << UF_PORT)) ?
    util::sview{p + u.field_data[UF_PORT].off, u.field_data[UF_PORT].len} : util::sview{};

  if(not port_str_.empty())
  {
    std::array<char, 32> buf;
    std::copy(port_str_.begin(), port_str_.end(), buf.begin());
    buf[port_str_.size()] = 0;
    port_ = std::atoi(buf.data());
  }
  else
  {
    port_ = bind_port(scheme_, u.port);
  }


  return *this;
}

///////////////////////////////////////////////////////////////////////////////
URI& URI::reset() {
  new (this) URI{};
  return *this;
}

///////////////////////////////////////////////////////////////////////////////
void URI::load_queries() {
  auto _ = query_;

  util::sview name  {};
  util::sview value {};
  util::sview::size_type base {0U};
  util::sview::size_type break_point {};

  while (true) {
    if ((break_point = _.find('=')) not_eq util::sview::npos) {
      name = _.substr(base, break_point);
      //-----------------------------------
      _.remove_prefix(name.length() + 1U);
    }
    else {
      break;
    }

    if ((break_point = _.find('&')) not_eq util::sview::npos) {
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
  return icase_equal(lhs.scheme(), rhs.scheme())
         and (lhs.userinfo() == rhs.userinfo())
         and icase_equal(lhs.host(), rhs.host())
         and lhs.port() == rhs.port()
         and lhs.path() == rhs.path()
         and lhs.query() == rhs.query()
         and lhs.fragment() == rhs.fragment();
}

///////////////////////////////////////////////////////////////////////////////
std::ostream& operator<< (std::ostream& output_device, const URI& uri) {
  return output_device << uri.to_string();
}

} //< namespace uri
