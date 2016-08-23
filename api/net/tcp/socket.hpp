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
#ifndef NET_TCP_SOCKET_HPP
#define NET_TCP_SOCKET_HPP

#include "common.hpp"

namespace net {
namespace tcp {

/**
  An IP address and a Port.
*/
class Socket {
public:
  /** Intialize an empty socket. */
  Socket() : address_(), port_(0)
  { address_.whole = 0; }

  /** Create a socket with a Address and Port. */
  Socket(Address address, port_t port)
    : address_(address), port_(port)
  {}

  /** Returns the Socket's address. */
  const Address address() const
  { return address_; }

  /** Returns the Socket's port. */
  port_t port() const
  { return port_; }

  /** Returns a string in the format "Address:Port". */
  std::string to_string() const
  { return address_.str() + ":" + std::to_string(port_); }

  bool is_empty() const
  { return (address_ == 0 and port_ == 0); }

  /** Standard comparators */
  bool operator ==(const Socket& s2) const
  {
    return address() == s2.address()
      and port() == s2.port();
  }

  bool operator <(const Socket& s2) const
  {
    return address() < s2.address()
           or (address() == s2.address() and port() < s2.port());
  }

  bool operator !=(const Socket& s2) const
  { return !(*this == s2); }

  bool operator >(const Socket& s2) const
  { return !(*this < s2); }

private:
  Address address_;
  port_t port_;

}; // < class Socket

} // < namespace tcp
} // < namespace net

#endif // < NET_TCP_SOCKET_HPP
