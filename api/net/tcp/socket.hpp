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

#include <sstream>
#include "common.hpp"

namespace net {
namespace tcp {

/*
  An IP address and a Port.
*/
class Socket {
public:
  /*
    Intialize an empty socket.
  */
  inline Socket() : address_(), port_(0) { address_.whole = 0; };

  /*
    Create a socket with a Address and Port.
  */
  inline Socket(Address address, port_t port) : address_(address), port_(port) {};

  /*
    Returns the Socket's address.
  */
  inline const Address address() const { return address_; }

  /*
    Returns the Socket's port.
  */
  inline port_t port() const { return port_; }

  /*
    Returns a string in the format "Address:Port".
  */
  std::string to_string() const {
    std::stringstream ss;
    ss << address_.str() << ":" << port_;
    return ss.str();
  }

  inline bool is_empty() const { return (address_.whole == 0 and port_ == 0); }

  /*
    Comparator used for vector.
  */
  inline bool operator ==(const Socket &s2) const {
    return address().whole == s2.address().whole
      and port() == s2.port();
  }

  /*
    Comparator used for map.
  */
  inline bool operator <(const Socket& s2) const {
    return address().whole < s2.address().whole
                             or (address().whole == s2.address().whole and port() < s2.port());
  }

private:
  //SocketID id_; // Maybe a hash or something. Not sure if needed (yet)
  Address address_;
  port_t port_;

}; // < class Socket

} // < namespace net
} // < namespace tcp

#endif // < NET_TCP_SOCKET_HPP
