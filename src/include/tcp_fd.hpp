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

#include "fd.hpp"
#include <net/inet4>

class TCP_FD : public FD {
public:
  using id_t = int;

  explicit TCP_FD(int id)
    : FD(id)
  {}

  int     read(void*, size_t) override;
  int     write(const void*, size_t) override;
  /** SOCKET */
  int     accept(struct sockaddr *__restrict__, socklen_t *__restrict__) override;
  int     bind(const struct sockaddr *, socklen_t) override;
  int     connect(const struct sockaddr *, socklen_t) override;
  /*
  virtual int     listen(int, int) { return -1; }
  virtual ssize_t recv(int, void *, size_t, int) { return 0; }
  virtual ssize_t recvfrom(int, void *restrict, size_t, int, struct sockaddr *restrict, socklen_t *restrict) { return 0; }
  virtual ssize_t recvmsg(int, struct msghdr *, int) { return 0; }
  virtual ssize_t send(int, const void *, size_t, int) { return 0; }
  virtual ssize_t sendmsg(int, const struct msghdr *, int) { return 0; }
  virtual ssize_t sendto(int, const void *, size_t, int, const struct sockaddr *, socklen_t) { return 0; }
  virtual int     setsockopt(int, int, int, const void *, socklen_t) { return -1; }
  virtual int     shutdown(int, int) { return -1; }
  */

protected:
  virtual ~TCP_FD() {}
private:
  net::tcp::Connection_ptr conn;
};

#endif
