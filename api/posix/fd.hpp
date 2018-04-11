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
#include <sys/types.h>
#include <fcntl.h>
#include <cstdarg>
#include <errno.h>

#define DEFAULT_ERR EPERM
/**
 * @brief File descriptor
 * @details
 *
 */
class FD {
public:
  using id_t = int;

  explicit FD(const int id)
    : id_(id), fflags(0)
  {}

  /** FILES **/
  virtual ssize_t read(void*, size_t) { return -DEFAULT_ERR; }
  virtual ssize_t readv(const struct iovec*, int) { return -DEFAULT_ERR; }
  virtual int     write(const void*, size_t) { return -DEFAULT_ERR; }
  virtual int     close() = 0;
  virtual int     fcntl(int, va_list);
  virtual int     ioctl(int, void*);

  /** SOCKET **/
  virtual long    accept(struct sockaddr *__restrict__, socklen_t *__restrict__) { return -ENOTSOCK; }
  virtual long    bind(const struct sockaddr *, socklen_t) { return -ENOTSOCK; }
  virtual long    connect(const struct sockaddr *, socklen_t) { return-ENOTSOCK; }
  virtual int     getsockopt(int, int, void *__restrict__, socklen_t *__restrict__);
  virtual long    listen(int) { return -ENOTSOCK; }
  virtual ssize_t recv(void *, size_t, int) { return 0; }
  virtual ssize_t recvfrom(void *__restrict__, size_t, int, struct sockaddr *__restrict__, socklen_t *__restrict__) { return 0; }
  virtual ssize_t recvmsg(struct msghdr *, int) { return 0; }
  virtual ssize_t send(const void *, size_t, int) { return 0; }
  virtual ssize_t sendmsg(const struct msghdr *, int) { return 0; }
  virtual ssize_t sendto(const void *, size_t, int, const struct sockaddr *, socklen_t) { return 0; }
  virtual int     setsockopt(int, int, const void *, socklen_t);
  virtual int     shutdown(int) { return -1; }

  // file-related
  virtual int   fchmod(mode_t) { return -1; }
  virtual int   fchmodat(const char *, mode_t, int) { return -1; }
  virtual long  fstat(struct stat *) { return -1; }
  virtual int   fstatat(const char *, struct stat *, int) { return -1; }
  virtual int   futimens(const struct timespec[2]) { return -1; }
  virtual int   utimensat(const char *, const struct timespec[2], int) { return -1; }
  virtual int   mkdirat(const char *, mode_t) { return -1; }
  virtual int   mkfifoat(const char *, mode_t) { return -1; }
  virtual int   mknodat(const char *, mode_t, dev_t) { return -1; }
  virtual off_t lseek(off_t, int) { return -DEFAULT_ERR; }

  // linux specific
  virtual long getdents(struct dirent*, unsigned int) { return -1; }

  id_t get_id() const noexcept { return id_; }

  virtual bool is_file() { return false; }
  virtual bool is_socket() { return false; }

  bool operator==(const FD& fd) const noexcept { return id_ == fd.id_; }
  bool operator!=(const FD& fd) const noexcept { return !(*this == fd); }

  bool is_blocking() const noexcept {
    return this->non_blocking == 0;
  }

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
