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

#include <common.cxx>
#include <arpa/inet.h>
#include <posix/arpa/inet.cpp>

const int TEST_ADDR = 0x0A00002A; // 10.0.0.42
const char* TEST_ADDRSTR = "10.0.0.42";

CASE("IPv4 address manipulation - str to addr")
{
  in_addr_t addr;

  addr = inet_addr(TEST_ADDRSTR);
  EXPECT(::ntohl(addr) == TEST_ADDR);

  addr = inet_addr("Invalid.Address");
  EXPECT(addr == -1);
}
CASE("IPv4 address manipulation - addr to str")
{
  struct in_addr addr;
  addr.s_addr = ::htonl(TEST_ADDR);

  char* addrstr = inet_ntoa(addr);
  EXPECT(strcmp(addrstr, TEST_ADDRSTR) == 0);
}
CASE("IPv4 address manipulation - addr to pointer")
{
  struct in_addr addr;
  addr.s_addr = ::htonl(TEST_ADDR);

  char addrstr[INET_ADDRSTRLEN];

  const char* res = inet_ntop(AF_INET, &addr, (char*)&addrstr, sizeof(addrstr));

  EXPECT(res == (char*)&addrstr);
  EXPECT(strcmp(addrstr, TEST_ADDRSTR) == 0);

  // Unsupported address family yields error
  res = inet_ntop(AF_UNIX, &addr, (char*)&addrstr, sizeof(addrstr));
  EXPECT(errno == EAFNOSUPPORT);
  EXPECT(res == nullptr);

  // Too small address buffer yeilds error
  char addrstr2[11];
  res = inet_ntop(AF_INET, &addr, (char*)&addrstr2, sizeof(addrstr2));
  EXPECT(errno == ENOSPC);

  // No buffer returns nullptr
  res = inet_ntop(AF_INET, &addr, nullptr, 0);
  EXPECT(res == nullptr);

}
CASE("IPv4 address manipulation - pointer to addr")
{
  struct in_addr addr;
  int res = 0;

  res = inet_pton(AF_INET, TEST_ADDRSTR, &addr);
  EXPECT(res > 0);
  EXPECT(addr.s_addr == htonl(TEST_ADDR));

  // Unsupported address family yields error
  res = inet_pton(AF_UNIX, TEST_ADDRSTR, &addr);
  EXPECT(res == -1);
  EXPECT(errno == EAFNOSUPPORT);

  // Unsupported IP4 format
  res = inet_pton(AF_INET, "256.666.666.1338", &addr);
  EXPECT(res == 0);

  // Unsupported IP4 format
  res = inet_pton(AF_INET, "255.255.0", &addr);
  EXPECT(res == 0);
}
