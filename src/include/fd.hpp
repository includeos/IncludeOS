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
#ifndef INCLUDE_FD_HPP
#define INCLUDE_FD_HPP

#include <sys/socket.h>

/**
 * @brief File descriptor
 * @details
 *
 */
class FD {
public:
  using id_t = int;

  explicit FD(int id)
    : id_(id)
  {}

  virtual int     read(void*, size_t) { return -1; }
  virtual int     write(const void*, size_t) { return -1; }
  /** SOCKET */
  virtual int     accept(struct sockaddr *__restrict__, socklen_t *__restrict__) { return -1; }
  virtual int     bind(const struct sockaddr *, socklen_t) { return -1; }
  virtual int     connect(const struct sockaddr *, socklen_t) { return -1; }
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

  int get_id() const noexcept { return id_; }

  virtual ~FD() {}
private:
  id_t id_;
};

#endif
