// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2017 Oslo and Akershus University College of Applied Sciences
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
#include <net/inet_common.hpp>
#include <info>
#include <cstdlib>
using namespace net;

static uint16_t safe_checksum(const void* buf, size_t length)
{
  const auto* buffer = (const uint16_t*) buf;

  union {
    uint32_t whole;
    uint16_t part[2];
  } sum{0};

  for (auto* it = buffer; it < buffer + length / 2; it++)
    sum.whole+= *it;

  // The odd-numbered case
  bool odd = length & 1;
  sum.whole += (odd) ? ((uint8_t*)buf)[length - 1] << 16 : 0;

  sum.whole = (uint32_t)sum.part[0] + sum.part[1];
  sum.part[0] += sum.part[1];
  return ~sum.whole;
}

static bool verify(const void* buffer, size_t length)
{
  for (size_t i = 0; i < length; i++)
    if (safe_checksum(buffer, i) != net::checksum(buffer, i))
        return false;
  return true;
}

CASE("Verify IP checksum using buffers of various lengths")
{
  char buffer[2048];

  for (size_t j = 0; j < 32; j++)
  {
    buffer[0] = 0;
    buffer[1] = 0;
    for (size_t i = 2; i < sizeof(buffer); i++)
        buffer[i] = rand() & 0xff;

    EXPECT(verify(buffer, sizeof(buffer)));

    // verify that checksum becomes zero
    auto csum = net::checksum(buffer, sizeof(buffer));
    memcpy(buffer, &csum, sizeof(csum));

    EXPECT(net::checksum(buffer, sizeof(buffer)) == 0);
  }
}

CASE("Verify adjusting checksum")
{
  char buffer[2048];
  for (size_t j = 0; j < 32; j++)
  {
    buffer[0] = 0;
    buffer[1] = 0;
    for (size_t i = 2; i < sizeof(buffer); i++)
      buffer[i] = rand() & 0xff;

    // Set checksum
    auto csum = net::checksum(buffer, sizeof(buffer));
    memcpy(buffer, &csum, sizeof(csum));

    // Create some random data
    char rndm[512];
    for (size_t i = 0; i < sizeof(rndm); i++)
      rndm[i] = rand() & 0xff;

    // A fixed point inside the buffer
    const size_t N = sizeof(buffer) / 2;

    // Adjust the checksum
    net::checksum_adjust((uint8_t*)buffer, &buffer[N], sizeof(rndm), rndm, sizeof(rndm));

    // Make sure to add the random data at the same place
    memcpy(&buffer[N], rndm, sizeof(rndm));

    // Checksum the buffer (without the csum)
    csum = net::checksum(&buffer[2], sizeof(buffer)-2);

    EXPECT(csum == *(uint16_t*)buffer);
  }
}
