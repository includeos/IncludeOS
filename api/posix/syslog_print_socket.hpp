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
#ifndef POSIX_SYSLOG_PRINT_SOCKET_HPP
#define POSIX_SYSLOG_PRINT_SOCKET_HPP

#include <posix/unix_fd_impl.hpp>

class Syslog_print_socket : public Unix_FD_impl {
public:
  Syslog_print_socket() = default;
  inline long    connect(const struct sockaddr *, socklen_t) override;
  inline ssize_t sendto(const void* buf, size_t, int fl,
                 const struct sockaddr* addr, socklen_t addrlen) override;
  inline int     close() override { return 0; }
  ~Syslog_print_socket() = default;
};

long Syslog_print_socket::connect(const struct sockaddr *, socklen_t)
{
  // connect doesnt really mean anything here
  return 0;
}

#include <os.hpp>
ssize_t Syslog_print_socket::sendto(const void* buf, size_t len, int /*fl*/,
                                    const struct sockaddr* /*addr*/,
                                    socklen_t /*addrlen*/)
{
  os::print(reinterpret_cast<const char*>(buf), len);
  return len;
}

#endif
