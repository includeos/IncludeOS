// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2016 Oslo and Akershus University College of Applied Sciences
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

#include <service>
#include <random>   // std::random_device
#include <fcntl.h>  // open
#include <unistd.h> // close, read
#include <cassert>
#include <info>
#include <map> // hist

static const size_t BUFLEN = 4096;
void Service::start()
{
  INFO("POSIX", "Operating on fd \"dev/(u)random\"");

  int fd = open("/dev/urandom", O_RDONLY);
  CHECKSERT(fd > 0, "open /dev/urandom returns fd");
  printf("fd: %d\n", fd);

  // close is *not* ignored!
  int res = close(fd);
  CHECKSERT(res == 0, "close does not return error");

  int rand_fd = open("/dev/random", O_RDONLY);
  CHECKSERT(rand_fd > 0, "open /dev/random returns fd");

  uint8_t a[BUFLEN];
  uint8_t b[BUFLEN];

  memset(a, 0, BUFLEN);
  memset(b, 0, BUFLEN);

  CHECKSERT(read(rand_fd, a, sizeof(a)) == sizeof(a), "read returns the correct length (%u)", BUFLEN);

  read(rand_fd, b, sizeof(b));
  CHECKSERT(memcmp(a, b, BUFLEN) != 0, "reading returns new data");

  auto now = time(0);
  CHECKSERT(write(rand_fd, &now, sizeof(now)) == sizeof(now), "write returns the correct length");


  INFO("std", "Using std::random_device");

  std::random_device rd;
  CHECKSERT(true, "Constructing random_device did not throw");

  // Open for improvements. Now only writes a histogram,
  // taken from http://en.cppreference.com/w/cpp/numeric/random/random_device
  INFO2("Throwing a standard dice 20 000 times...");
  std::map<int, int> hist;
  std::uniform_int_distribution<int> dist(1, 6);
  for (int n = 0; n < 10000; ++n) {
      ++hist[dist(rd)]; // note: demo only: the performance of many
                        // implementations of random_device degrades sharply
                        // once the entropy pool is exhausted. For practical use
                        // random_device is generally only used to seed
                        // a PRNG such as mt19937
  }
  for (auto p : hist) {
      INFO2("%u : %s", p.first, std::string(p.second/100, '*').c_str());
  }

  printf("SUCCESS\n");
}
