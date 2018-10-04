// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2018 IncludeOS AS, Oslo, Norway
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
#ifndef POSIX_UNIX_FD_HPP
#define POSIX_UNIX_FD_HPP

#include "sockfd.hpp"
#include "unix_fd_impl.hpp"
#include <sys/socket.h>

class Unix_FD : public SockFD {
public:
  using Impl = Unix_FD_impl;
  Unix_FD(int id, int type)
    : SockFD(id), type_(type)
  {
  }

  /** SOCKET */
  long    connect(const struct sockaddr *, socklen_t) override;
  ssize_t sendto(const void* buf, size_t, int fl,
                 const struct sockaddr* addr, socklen_t addrlen) override;
  int     close() override;
private:
  Impl* impl = nullptr;
  const int type_; // it's probably gonna be necessary
                   // to tell if socket is stream or dgram

  long set_impl_if_needed(const struct sockaddr* addr, socklen_t addrlen);
};

#endif
