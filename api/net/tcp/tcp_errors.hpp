
#pragma once
#ifndef NET_TCP_TCP_ERRORS_HPP
#define NET_TCP_TCP_ERRORS_HPP

#include <stdexcept>
#include "options.hpp"

namespace net {
namespace tcp {

class TCPException : public std::runtime_error {
public:
  TCPException(const std::string& error) : std::runtime_error(error) {};
  virtual ~TCPException() {};
};

/*
  Exception for Bad TCP Header Option (TCP::Option)
*/
class TCPBadOptionException : public TCPException {
public:
  TCPBadOptionException(Option::Kind kind, const std::string& error)
    : TCPException("Bad Option [" + Option::kind_string(kind) + "]: " + error),
      kind_(kind)
  {}

  Option::Kind kind();
private:
  Option::Kind kind_;
};

} // < namespace tcp
} // < namespace net

#endif // < NET_TCP_TCP_ERRORS_HPP
