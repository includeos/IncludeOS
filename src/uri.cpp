#include <regex>
#include <iostream>

#include <uri.hpp>

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
  port_ = port_str_.begin ? static_cast<uint16_t>(std::stoi(port_str())) : 0;
}

///////////////////////////////////////////////////////////////////////////////
std::string URI::userinfo() const {
  return uri_str_.substr(userinfo_.begin, userinfo_.end);
}

///////////////////////////////////////////////////////////////////////////////
std::string URI::host() const {
  return uri_str_.substr(host_.begin, host_.end);
}

///////////////////////////////////////////////////////////////////////////////
std::string URI::port_str() const {
  return uri_str_.substr(port_str_.begin, port_str_.end);
}

///////////////////////////////////////////////////////////////////////////////
uint16_t URI::port() const noexcept {
  return port_;
}

///////////////////////////////////////////////////////////////////////////////
std::string URI::path() const {
  return uri_str_.substr(path_.begin, path_.end);
}

///////////////////////////////////////////////////////////////////////////////
std::string URI::query() const {
  return uri_str_.substr(query_.begin, query_.end);
}

///////////////////////////////////////////////////////////////////////////////
std::string URI::fragment() const {
  return uri_str_.substr(fragment_.begin, fragment_.end);
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
    "^([\\w]+)?(\\://)?"        //< scheme
    "(([^:@]+)(\\:([^@]+))?@)?" //< username && password
    "([^/:?#]+)?(\\:(\\d+))?"   //< hostname && port
    "([^?#]+)"                  //< path
    "(\\?([^#]*))?"             //< query
    "(#(.*))?$"                 //< fragment
  };

  std::smatch uri_parts;

  if (std::regex_match(uri, uri_parts, uri_pattern_matcher)) {
    path_     = Span_t(uri_parts.position(10), uri_parts.length(10));

    userinfo_ = uri_parts.length(3)  ? Span_t(uri_parts.position(3),  uri_parts.length(3))  : zero_span_;
    host_     = uri_parts.length(7)  ? Span_t(uri_parts.position(7),  uri_parts.length(7))  : zero_span_;
    port_str_ = uri_parts.length(9)  ? Span_t(uri_parts.position(9),  uri_parts.length(9))  : zero_span_;
    query_    = uri_parts.length(11) ? Span_t(uri_parts.position(11), uri_parts.length(11)) : zero_span_;
    fragment_ = uri_parts.length(12) ? Span_t(uri_parts.position(12), uri_parts.length(12)) : zero_span_;
  }
}

///////////////////////////////////////////////////////////////////////////////
std::ostream& uri::operator<< (std::ostream& out, const URI& uri) {
  return out << uri.to_string();
}
