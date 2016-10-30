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
#ifndef POSIX_NETINET_IN_H
#define POSIX_NETINET_IN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <sys/socket.h>

typedef uint16_t  in_port_t;
typedef uint32_t  in_addr_t;

struct in_addr
{
  in_addr_t s_addr;
};
struct sockaddr_in
{
  sa_family_t     sin_family;
  in_port_t       sin_port;
  struct in_addr  sin_addr;
};


struct in6_addr
{
  uint8_t s6_addr[16];
};
struct sockaddr_in6
{
  sa_family_t      sin6_family;
  uint16_t         sin6_port;
  uint32_t         sin6_flowinfo;
  struct in6_addr  sin6_addr;
  uint32_t         sin6_scope_id;
};

extern const struct in6_addr in6addr_any;
extern const struct in6_addr in6addr_loopback;

/// fixme: use actual inet protocol values
#define IPPROTO_IP        0
#define IPPROTO_ICMP      1
#define IPPROTO_TCP       6
#define IPPROTO_UDP      17
#define IPPROTO_IPV6     41
#define IPPROTO_RAW     255

#define INADDR_ANY        0x0
#define INADDR_BROADCAST  0xffffffff
#define	INADDR_NONE		    0xffffffff
#define INADDR_LOOPBACK   0x7f000001

#define INET_ADDRSTRLEN   16
#define INET6_ADDRSTRLEN  46

#include "../arpa/inet.h"

#ifdef __cplusplus
}
#endif

#endif
