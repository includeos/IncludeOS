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

using namespace net;

/**
 * @note: shortcut, not sufficent.
 * see: http://pubs.opengroup.org/onlinepubs/9699919799/functions/inet_addr.html#
 */
in_addr_t inet_addr(const char* cp)
{
  try {
    const auto addr = ip4::Addr{cp};
    return addr.whole;
  }
  catch(const ip4::Invalid_address&) {
    return (in_addr_t)(-1);
  }
}

/**
 * @note: shortcut, not sufficent.
 * see: http://pubs.opengroup.org/onlinepubs/9699919799/functions/inet_addr.html#
 */
char* inet_ntoa(struct in_addr in)
{
  const auto addr = ip4::Addr{in.s_addr};
  auto str = addr.to_string();
  return const_cast<char*>(str.c_str());
}

/**
 * @note: http://pubs.opengroup.org/onlinepubs/9699919799/functions/inet_ntop.html#
 */
const char* inet_ntop(int, const void *__restrict__, char *__restrict__, socklen_t)
{
  assert(false && "Not implemented");
}

/**
 * @note: http://pubs.opengroup.org/onlinepubs/9699919799/functions/inet_ntop.html#
 */
int inet_pton(int, const char *__restrict__, void *__restrict__)
{
  assert(false && "Not implemented");
}
