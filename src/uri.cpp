#include <uri>
#include <regex>
#include <iostream>

using namespace uri;

///////////////////////////////////////////////////////////////////////////////
const URI::Span_t URI::zero_span_;

///////////////////////////////////////////////////////////////////////////////
URI::URI(const char* uri)
  : URI{std::string{uri}}
{}

///////////////////////////////////////////////////////////////////////////////
URI::URI(const std::string& uri)
  : uri_str_{uri}
  , port_{}
{
  parse(uri_str_);
  load_queries();
}

///////////////////////////////////////////////////////////////////////////////
const std::string& URI::scheme() const {
  static std::string scheme;

  if (scheme.empty()) {
    scheme = uri_str_.substr(scheme_.begin, scheme_.end);
  }

  return scheme;
}

///////////////////////////////////////////////////////////////////////////////
const std::string& URI::userinfo() const {
  static std::string userinfo;

  if (userinfo.empty()) {
    userinfo = uri_str_.substr(userinfo_.begin, userinfo_.end);
  }

  return userinfo;
}

///////////////////////////////////////////////////////////////////////////////
const std::string& URI::host() const {
  static std::string host;

  if (host.empty()) {
    host = uri_str_.substr(host_.begin, host_.end);
  }

  return host;
}

///////////////////////////////////////////////////////////////////////////////
const std::string& URI::port_str() const {
  static std::string port;

  if (port.empty()) {
    port = uri_str_.substr(port_str_.begin, port_str_.end);
  }

  return port;
}

///////////////////////////////////////////////////////////////////////////////
uint16_t URI::port() const noexcept {
  if (not port_) {
    port_ = port_str_.begin ? static_cast<uint16_t>(std::stoi(port_str())) : 0;
  }
  return port_;
}

///////////////////////////////////////////////////////////////////////////////
const std::string& URI::path() const {
  static std::string path;

  if (path.empty()) {
    path = uri_str_.substr(path_.begin, path_.end);
  }

  return path;
}

///////////////////////////////////////////////////////////////////////////////
const std::string& URI::query() const {
  static std::string query;

  if (query.empty()){
    query = uri_str_.substr(query_.begin, query_.end);
  }

  return query;
}

///////////////////////////////////////////////////////////////////////////////
const std::string& URI::fragment() const {
  static std::string fragment;

  if (fragment.empty()) {
    fragment = uri_str_.substr(fragment_.begin, fragment_.end);
  }

  return fragment;
}

///////////////////////////////////////////////////////////////////////////////
std::string URI::query(const std::string& key) {
  static std::string no_entry_value;
  auto target = queries_.find(key);
  return (target not_eq queries_.end()) ? decode(target->second) : no_entry_value;
}

///////////////////////////////////////////////////////////////////////////////
std::string URI::to_string() const{
  return uri_str_;
}

///////////////////////////////////////////////////////////////////////////////
URI::operator std::string () const {
  return uri_str_;
}

///////////////////////////////////////////////////////////////////////////////
void URI::parse(const std::string& uri) {
  static const std::regex uri_pattern_matcher
  {
    "^([a-zA-z]+[\\w\\+\\-\\.]+)?(\\://)?" //< scheme
    "(([^:@]+)(\\:([^@]+))?@)?"            //< username && password
    "([^/:?#]+)?(\\:(\\d+))?"              //< hostname && port
    "([^?#]+)"                             //< path
    "(\\?([^#]*))?"                        //< query
    "(#(.*))?$"                            //< fragment
  };

  static std::smatch uri_parts;

  if (std::regex_match(uri, uri_parts, uri_pattern_matcher)) {
    path_     = Span_t(uri_parts.position(10), uri_parts.length(10));

    scheme_   = uri_parts.length(1)  ? Span_t(uri_parts.position(1),  uri_parts.length(1))  : zero_span_;
    userinfo_ = uri_parts.length(3)  ? Span_t(uri_parts.position(3),  uri_parts.length(3))  : zero_span_;
    host_     = uri_parts.length(7)  ? Span_t(uri_parts.position(7),  uri_parts.length(7))  : zero_span_;
    port_str_ = uri_parts.length(9)  ? Span_t(uri_parts.position(9),  uri_parts.length(9))  : zero_span_;
    query_    = uri_parts.length(11) ? Span_t(uri_parts.position(11), uri_parts.length(11)) : zero_span_;
    fragment_ = uri_parts.length(13) ? Span_t(uri_parts.position(13), uri_parts.length(13)) : zero_span_;
  }
}

///////////////////////////////////////////////////////////////////////////////
void URI::load_queries() {
  static const std::regex queries_tokenizer {"[^?=&]+"};

  auto& queries = query();

  auto position = std::sregex_iterator(queries.begin(), queries.end(), queries_tokenizer);
  auto end      = std::sregex_iterator();

  while (position not_eq end) {
    auto key = (*position).str();

    if ((++position) not_eq end) {
      queries_[key] = (*position++).str();
      continue;
    } else {
      queries_[key];
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
std::ostream& uri::operator<< (std::ostream& out, const URI& uri) {
  return out << uri.to_string();
}
