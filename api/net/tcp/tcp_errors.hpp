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
