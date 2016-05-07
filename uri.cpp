#include <uri.hpp>
#include <iostream>

using namespace uri;

/*
URI::URI(gsl::span<const char> data) :
  uri_data_ {data}
{
  path_ = data;
  }*/

URI::URI(std::string data) :
  uri_str_ {data}
{
  path_ = uri_str_;
  uri_data_ = uri_str_;
}


std::string URI::path() const {
  return std::string{path_.begin(), path_.end()};
}


std::string URI::to_string() const{
  return std::string{uri_data_.begin(), uri_data_.end()};
}


std::ostream& uri::operator<< (std::ostream& out, const URI& uri) {
  out << uri.to_string();
  return out;
}




/*
URI::URI(const std::string& data) :
  uri_data_ {data}
  {} */
