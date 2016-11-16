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
#include <cstdarg>

/**
 * @brief File descriptor
 * @details
 *
 */
class FD {
public:
  using id_t = int;

  explicit FD(const int id)
    : id_(id)
  {}

  /** FILES **/
  virtual int     read(void*, size_t) { return -1; }
  virtual int     write(const void*, size_t) { return -1; }
  virtual int     close() = 0;
  virtual int     fcntl(int, va_list);

  /** SOCKET **/
  virtual int     accept(struct sockaddr *__restrict__, socklen_t *__restrict__) { return -1; }
  virtual int     bind(const struct sockaddr *, socklen_t) { return -1; }
  virtual int     connect(const struct sockaddr *, socklen_t) { return -1; }
  virtual int     getsockopt(int, int, void *__restrict__, socklen_t *__restrict__);
  virtual int     listen(int) { return -1; }
  virtual ssize_t recv(void *, size_t, int) { return 0; }
  virtual ssize_t recvfrom(void *__restrict__, size_t, int, struct sockaddr *__restrict__, socklen_t *__restrict__) { return 0; }
  virtual ssize_t recvmsg(struct msghdr *, int) { return 0; }
  virtual ssize_t send(const void *, size_t, int) { return 0; }
  virtual ssize_t sendmsg(const struct msghdr *, int) { return 0; }
  virtual ssize_t sendto(const void *, size_t, int, const struct sockaddr *, socklen_t) { return 0; }
  virtual int     setsockopt(int, int, const void *, socklen_t);
  virtual int     shutdown(int) { return -1; }

  id_t get_id() const noexcept { return id_; }

  virtual bool is_file() { return false; }
  virtual bool is_socket() { return false; }

  bool operator==(const FD& fd) const noexcept { return id_ == fd.id_; }
  bool operator!=(const FD& fd) const noexcept { return !(*this == fd); }

  virtual ~FD() {}

private:
  const id_t id_;
  int dflags;
  union {
    struct {
      int   non_blocking : 1;
    };
    int fflags;
  };
};

#endif
