#include <uri.hpp>
#include <iostream>

using namespace uri;

void URI::init_spans() {
  path_ = {0, uri_str_.size()};
  uri_data_ = {0, uri_str_.size()};
}

URI::URI(const std::string&& data) :
  uri_str_ {std::forward<const std::string>(data)}
{
  init_spans();
}

URI::URI(const char* data) :
  uri_str_ {data}
{
  init_spans();
}


std::string URI::path() const {
  return uri_str_.substr(path_.begin, path_.end);
}

std::string URI::to_string() const{
  return uri_str_;
}

std::ostream& uri::operator<< (std::ostream& out, const URI& uri) {
  out << uri.to_string();
  return out;
}
