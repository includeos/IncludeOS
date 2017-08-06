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

#pragma once
#ifndef NET_SOCKET_HPP
#define NET_SOCKET_HPP

#include <net/ip4/addr.hpp>

namespace net {

/**
 * An IP address and port
 */
class Socket {
public:

  using Address = ip4::Addr;
  using port_t = uint16_t;

  struct pair_hash {
    std::size_t operator () (const Socket& s) const {
      auto h1 = std::hash<Address>{}(s.address());
      auto h2 = std::hash<port_t>{}(s.port());
      return h1 ^ h2;
    }
  };

  /**
   * Constructor
   *
   * Intialize an empty socket <0.0.0.0:0>
   */
  constexpr Socket() noexcept
    : address_{}, port_{}
  {}

  /**
   * Constructor
   *
   * Create a socket with an address and port
   *
   * @param address
   *  The host's network address
   *
   * @param port
   *  The port associated with the process
   */
  constexpr Socket(const Address address, const port_t port) noexcept
    : address_{address}, port_{port}
  {}

  /**
   * Get the socket's network address
   *
   * @return The socket's network address
   */
  constexpr Address address() const noexcept
  { return address_; }

  /**
   * Get the socket's port value
   *
   * @return The socket's port value
   */
  constexpr port_t port() const noexcept
  { return port_; }

  /**
   * Get a string representation of this class
   *
   * @return A string representation of this class
   */
  std::string to_string() const
  { return address_.str() + ":" + std::to_string(port_); }

  /**
   * Check if this socket is empty <0.0.0.0:0>
   *
   * @return true if this socket is empty, false otherwise
   */
  constexpr bool is_empty() const noexcept
  { return (address_ == 0) and (port_ == 0); }

  /**
   * Operator to check for equality relationship
   *
   * @param other
   *  The socket to check for equality relationship
   *
   * @return true if the specified socket is equal, false otherwise
   */
  constexpr bool operator==(const Socket& other) const noexcept
  {
    return (address() == other.address())
       and (port() == other.port());
  }

  /**
   * Operator to check for inequality relationship
   *
   * @param other
   *  The socket to check for inequality relationship
   *
   * @return true if the specified socket is not equal, false otherwise
   */
  constexpr bool operator!=(const Socket& other) const noexcept
  { return not (*this == other); }

  /**
   * Operator to check for less-than relationship
   *
   * @param other
   *  The socket to check for less-than relationship
   *
   * @return true if this socket is less-than the specified socket,
   * false otherwise
   */
  constexpr bool operator<(const Socket& other) const noexcept
  {
    return (address() < other.address())
        or ((address() == other.address()) and (port() < other.port()));
  }

  /**
   * Operator to check for greater-than relationship
   *
   * @param other
   *  The socket to check for greater-than relationship
   *
   * @return true if this socket is greater-than the specified socket,
   * false otherwise
   */
  constexpr bool operator>(const Socket& other) const noexcept
  { return not (*this < other); }

private:
  Address address_;
  port_t  port_;

}; //< class Socket

class Quadruple {

public:
  Quadruple() noexcept
    : source_{}, destination_{}
  {}

  Quadruple(const Socket::Address src_address, const Socket::port_t src_port,
    const Socket::Address dst_address, const Socket::port_t dst_port) noexcept
    : source_{src_address, src_port}, destination_{dst_address, dst_port}
  {}

  const Socket& source() const noexcept
  { return source_; }

  const Socket& destination() const noexcept
  { return destination_; }

private:
  Socket source_;
  Socket destination_;

};  //< class Quadruple

} //< namespace net

#endif //< NET_SOCKET_HPP
