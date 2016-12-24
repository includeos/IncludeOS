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

#include <arpa/inet.h>
#include <net/ip4/addr.hpp>
#include <common>

/**
 * @note: shortcut, not sufficent.
 * see: http://pubs.opengroup.org/onlinepubs/9699919799/functions/inet_addr.html#
 */
in_addr_t inet_addr(const char* cp)
{
  try {
    const auto addr = net::ip4::Addr{cp};
    return addr.whole;
  }
  catch(const net::ip4::Invalid_address&) {
    return (in_addr_t)(-1);
  }
}

char* inet_ntoa(struct in_addr ina)
{
  static char buffer[INET_ADDRSTRLEN];
  unsigned char* byte = (unsigned char *)&ina;

  sprintf(buffer, "%hhu.%hhu.%hhu.%hhu",
          byte[0], byte[1], byte[2], byte[3]);
  return buffer;
}

/**
 * @note: Missing IPv6 support
 */
const char* inet_ntop(int af, const void *__restrict__ src, char *__restrict__ dst, socklen_t size)
{
  if(UNLIKELY(dst == nullptr))
    return nullptr;

  // IPv4
  if(af == AF_INET)
  {
    if(LIKELY(size >= INET_ADDRSTRLEN))
    {
      unsigned char* byte = (unsigned char*)src;

      sprintf(dst, "%hhu.%hhu.%hhu.%hhu",
          byte[0], byte[1], byte[2], byte[3]);

      return dst;
    }
    else
    {
      errno = ENOSPC;
    }
  }
  else if(af == AF_INET6)
  {
    // add me ;(
    errno = EAFNOSUPPORT;
  }
  else
  {
    errno = EAFNOSUPPORT;
  }
  return nullptr;
}

/**
 * @note: Missing IPv6 support
 */
int inet_pton(int af, const char *__restrict__ src, void *__restrict__ dst)
{
  // IPv4
  if(af == AF_INET)
  {
    try
    {
      const auto addr = net::ip4::Addr{src};
      memcpy(dst, &addr.whole, INET_ADDRSTRLEN);
      return 1;
    }
    catch(const net::ip4::Invalid_address&)
    {
      return 0;
    }
  }
  else if(af == AF_INET6)
  {
    // add me ;(
    errno = EAFNOSUPPORT;
  }
  else
  {
    errno = EAFNOSUPPORT;
  }
  return -1;

}
