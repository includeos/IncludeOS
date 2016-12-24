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
#include <ringbuffer>

struct TCP_FD_Conn;
struct TCP_FD_Listen;

class TCP_FD : public SockFD {
public:
  using id_t = int;

  explicit TCP_FD(int id)
    : SockFD(id)
  {}

  int     read(void*, size_t) override;
  int     write(const void*, size_t) override;
  int     close() override;
  
  /** SOCKET */
  int     bind(const struct sockaddr *, socklen_t) override;
  int     listen(int) override;
  int     accept(struct sockaddr *__restrict__, socklen_t *__restrict__) override;
  int     connect(const struct sockaddr *, socklen_t) override;

  ssize_t send(const void *, size_t, int fl) override;
  ssize_t recv(void*, size_t, int fl) override;
  
  int     shutdown(int) override;

  bool is_listener() const noexcept {
    return ld != nullptr;
  }
  bool is_connection() const noexcept {
    return cd != nullptr;
  }

  on_read_func   get_default_read_func()   override;
  on_write_func  get_default_write_func()  override;
  on_except_func get_default_except_func() override;
  
  ~TCP_FD() {}
private:
  TCP_FD_Conn* cd = nullptr;
  TCP_FD_Listen*  ld = nullptr;
  // sock opts
  bool non_blocking = false;
  
  friend struct TCP_FD_Listen;
};

struct TCP_FD_Conn
{
  TCP_FD_Conn(net::tcp::Connection_ptr c)
    : conn(c), readq(16484)
  {}
  
  void recv_to_ringbuffer(net::tcp::buffer_t, size_t);
  void set_default_read();
  
  ssize_t send(const void *, size_t, int fl);
  ssize_t recv(void*, size_t, int fl);
  int     close();
  int     shutdown(int);
  
  net::tcp::Connection_ptr conn;
  RingBuffer readq;
};

struct TCP_FD_Listen
{
  TCP_FD_Listen(net::tcp::Listener& l)
    : listener(l)
  {}
  
  int close();
  int listen(int);
  int accept(struct sockaddr *__restrict__, socklen_t *__restrict__);
  int shutdown(int);
  
  net::tcp::Listener& listener;
  std::deque<net::tcp::Connection_ptr> connq;
};

#endif
