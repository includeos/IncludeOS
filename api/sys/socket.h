// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
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
#ifndef POSIX_SYS_SOCKET_H
#define POSIX_SYS_SOCKET_H

#include <sys/types.h>
#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t socklen_t;
typedef uint8_t sa_family_t;

struct sockaddr
{
  sa_family_t  sa_family;  // Address family.
  char         sa_data[];  // Socket address (variable-length data).
};
struct sockaddr_in
{
  sa_family_t  sin_family;
  uint32_t     sin_addr;
  uint16_t     sin_port;
};

struct sockaddr_storage
{
  sa_family_t  sa_family;  // Address family.
  char         sa_data[16] __attribute__((aligned(16)));
};

struct msghdr
{
  void          *msg_name;        // Optional address.
  socklen_t      msg_namelen;     // Size of address.
  struct iovec  *msg_iov;         // Scatter/gather array.
  int            msg_iovlen;      // Members in msg_iov.
  void          *msg_control;     // Ancillary data; see below.
  socklen_t      msg_controllen;  // Ancillary data buffer len.
  int            msg_flags;       // Flags on received message.
};

struct cmsghdr
{
  socklen_t  cmsg_len;    // Data byte count, including the cmsghdr.
  int        cmsg_level;  // Originating protocol.
  int        cmsg_type;   // Protocol-specific type.
};

struct linger
{
  int  l_onoff;   // Indicates whether linger option is enabled.
  int  l_linger;  // Linger time, in seconds.
};

#define SOCK_STREAM     0
#define SOCK_DGRAM      1
#define SOCK_RAW        2
#define SOCK_SEQPACKET  3

#define SOL_SOCKET    322

#define SO_ACCEPTCONN  16
#define SO_BROADCAST   17
#define SO_DEBUG       18
#define SO_DONTROUTE   19
#define SO_ERROR       20
#define SO_KEEPALIVE   21
#define SO_LINGER      22
#define SO_OOBINLINE   23
#define SO_RCVBUF      24
#define SO_RCVLOWAT    25
#define SO_RCVTIMEO    26
#define SO_REUSEADDR   27
#define SO_SNDBUF      28
#define SO_SNDLOWAT    29
#define SO_TYPE        30

#define SOMAXCONN      64

#define AF_UNIX         0
#define AF_LOCAL  AF_UNIX
#define AF_INET         1
#define AF_INET6        2


int     accept(int socket, struct sockaddr *address,
               socklen_t *address_len);
int     bind(int socket, const struct sockaddr *address,
             socklen_t address_len);
int     connect(int socket, const struct sockaddr *address,
                socklen_t address_len);
int     getpeername(int socket, struct sockaddr *address,
                    socklen_t *address_len);
int     getsockname(int socket, struct sockaddr *address,
                    socklen_t *address_len);
int     getsockopt(int socket, int level, int option_name,
                   void *option_value, socklen_t *option_len);
int     listen(int socket, int backlog);
ssize_t recv(int socket, void *buffer, size_t length, int flags);
ssize_t recvfrom(int socket, void *buffer, size_t length,
                 int flags, struct sockaddr *address, socklen_t *address_len);
ssize_t recvmsg(int socket, struct msghdr *message, int flags);
ssize_t send(int socket, const void *message, size_t length, int flags);
ssize_t sendmsg(int socket, const struct msghdr *message, int flags);
ssize_t sendto(int socket, const void *message, size_t length, int flags,
               const struct sockaddr *dest_addr, socklen_t dest_len);
int     setsockopt(int socket, int level, int option_name,
                   const void *option_value, socklen_t option_len);
int     shutdown(int socket, int how);
int     socket(int domain, int type, int protocol);
int     socketpair(int domain, int type, int protocol,
                   int socket_vector[2]);

#ifdef __cplusplus
}
#endif
#endif
