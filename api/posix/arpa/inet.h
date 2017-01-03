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
#ifndef POSIX_ARPA_INET_H
#define POSIX_ARPA_INET_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../netinet/in.h"
#include <stdint.h>

#ifdef ntohs
#undef ntohs
#endif
#ifdef htons
#undef htons
#endif
#ifdef ntohl
#undef ntohl
#endif
#ifdef htonl
#undef htonl
#endif
#ifdef ntohll
#undef ntohll
#endif
#ifdef htonll
#undef htonll
#endif

inline uint16_t ntohs(const uint16_t n) {
  return __builtin_bswap16(n);
}

inline uint16_t htons(const uint16_t n) {
  return __builtin_bswap16(n);
}

inline uint32_t ntohl(const uint32_t n) {
  return __builtin_bswap32(n);
}

inline uint32_t htonl(const uint32_t n) {
  return __builtin_bswap32(n);
}

inline uint64_t ntohll(const uint64_t n) {
  return __builtin_bswap64(n);
}

inline uint64_t htonll(const uint64_t n) {
  return __builtin_bswap64(n);
}

in_addr_t    inet_addr(const char *);
char        *inet_ntoa(struct in_addr);
const char  *inet_ntop(int, const void *__restrict__, char *__restrict__,
                 socklen_t);
int          inet_pton(int, const char *__restrict__, void *__restrict__);

#ifdef __cplusplus
}
#endif

#endif
