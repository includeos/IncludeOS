#include <uri>
#include <regex>
#include <iostream>

using namespace uri;

///////////////////////////////////////////////////////////////////////////////
URI::URI(const char* uri)
  : URI{std::string{uri}}
{}

///////////////////////////////////////////////////////////////////////////////
URI::URI(const std::string& uri)
  : uri_str_{decode(uri)}
  , port_{}
{
  parse();
}

///////////////////////////////////////////////////////////////////////////////
const std::string& URI::scheme() const noexcept {
  return scheme_;
}

///////////////////////////////////////////////////////////////////////////////
const std::string& URI::userinfo() const noexcept {
  return userinfo_;
}

///////////////////////////////////////////////////////////////////////////////
const std::string& URI::host() const noexcept {
  return host_;
}

///////////////////////////////////////////////////////////////////////////////
const std::string& URI::port_str() const noexcept {
  return port_str_;
}

///////////////////////////////////////////////////////////////////////////////
uint16_t URI::port() const noexcept {
  if ((not port_) && (not port_str_.empty())) {
    port_ = static_cast<uint16_t>(std::stoi(port_str_));
  }
  return port_;
}

///////////////////////////////////////////////////////////////////////////////
const std::string& URI::path() const noexcept {
  return path_;
}

///////////////////////////////////////////////////////////////////////////////
const std::string& URI::query() const noexcept {
  return query_;
}

///////////////////////////////////////////////////////////////////////////////
const std::string& URI::fragment() const noexcept {
  return fragment_;
}

///////////////////////////////////////////////////////////////////////////////
const std::string& URI::query(const std::string& key) {
  static bool queries_not_loaded {true};
  static std::string no_entry_value;

  if (queries_not_loaded) {
    load_queries();
    queries_not_loaded = false;
  }

  auto target = queries_.find(key);

  return (target not_eq queries_.end()) ? target->second : no_entry_value;
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

  if (std::regex_match(uri_str_, uri_parts, uri_pattern_matcher)) {
    path_     = uri_parts[10].str();

    scheme_   = uri_parts.length(1)  ? uri_parts[1].str()  : "";
    userinfo_ = uri_parts.length(3)  ? uri_parts[3].str()  : "";
    host_     = uri_parts.length(7)  ? uri_parts[7].str()  : "";
    port_str_ = uri_parts.length(9)  ? uri_parts[9].str()  : "";
    query_    = uri_parts.length(11) ? uri_parts[11].str() : "";
    fragment_ = uri_parts.length(13) ? uri_parts[13].str() : "";
  }
}

///////////////////////////////////////////////////////////////////////////////
void URI::load_queries() {
  static const std::regex queries_tokenizer {"[^?=&]+"};

  auto position = std::sregex_iterator(query_.begin(), query_.end(), queries_tokenizer);
  auto end      = std::sregex_iterator();

  while (position not_eq end) {
    auto key = (*position).str();

    if ((++position) not_eq end) {
      queries_[key] = (*position++).str();
    } else {
      queries_[key];
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
std::ostream& uri::operator<< (std::ostream& out, const URI& uri) {
  return out << uri.to_string();
}
