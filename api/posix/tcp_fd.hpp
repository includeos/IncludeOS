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
#ifndef INCLUDE_TCP_FD_HPP
#define INCLUDE_TCP_FD_HPP

#include "sockfd.hpp"

struct TCP_FD_Conn;
struct TCP_FD_Listen;

class TCP_FD : public SockFD {
public:
  using id_t = int;

  explicit TCP_FD(int id)
    : SockFD(id)
  {}

  ssize_t read(void*, size_t) override;
  int     write(const void*, size_t) override;
  int     close() override;

  /** SOCKET */
  long    bind(const struct sockaddr *, socklen_t) override;
  long    listen(int) override;
  long    accept(struct sockaddr *__restrict__, socklen_t *__restrict__) override;
  long    connect(const struct sockaddr *, socklen_t) override;

  ssize_t send(const void *, size_t, int fl) override;
  ssize_t sendto(const void *, size_t, int fl,
                 const struct sockaddr*, socklen_t) override;
  ssize_t recv(void*, size_t, int fl) override;
  ssize_t recvfrom(void*, size_t, int fl, struct sockaddr*, socklen_t *) override;

  int     shutdown(int) override;

  bool is_listener() const noexcept {
    return ld != nullptr;
  }
  bool is_connection() const noexcept {
    return cd != nullptr;
  }
  inline net::tcp::Connection_ptr get_connection() noexcept;
  inline net::tcp::Listener& get_listener() noexcept;
  inline bool has_connq();

  ~TCP_FD() {}
private:
  std::unique_ptr<TCP_FD_Conn> cd = nullptr;
  TCP_FD_Listen* ld = nullptr;

  friend struct TCP_FD_Listen;
};

struct TCP_FD_Conn
{
  TCP_FD_Conn(net::tcp::Connection_ptr c);

  void retrieve_buffer();
  void set_default_read();

  ssize_t send(const void *, size_t, int fl);
  ssize_t recv(void*, size_t, int fl);
  int     close();
  int     shutdown(int);

  std::string to_string() const { return conn->to_string(); }

  net::tcp::Connection_ptr conn;
  net::tcp::buffer_t buffer;
  size_t buf_offset;
  bool recv_disc = false;
};

struct TCP_FD_Listen
{
  TCP_FD_Listen(net::tcp::Listener& l)
    : listener(l)
  {}

  int close();
  long listen(int);
  long accept(struct sockaddr *__restrict__, socklen_t *__restrict__);
  int shutdown(int);

  std::string to_string() const { return listener.to_string(); }

  net::tcp::Listener& listener;
  std::deque<std::unique_ptr<TCP_FD_Conn>> connq;
};

inline net::tcp::Connection_ptr TCP_FD::get_connection() noexcept {
  return cd->conn;
}
inline net::tcp::Listener& TCP_FD::get_listener() noexcept {
  return ld->listener;
}
inline bool TCP_FD::has_connq()
{
  return !ld->connq.empty();
}

#endif
