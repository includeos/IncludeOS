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
#include <net/buffer_store.hpp>
#include <stdlib.h>

using namespace net;
#define BUFFER_CNT 32
#define BUFFER_SZ  2048

CASE("Randomly release bufferstore buffers")
{
  // how many bufferstores we will force out
  static const int BS_CHAINS = 32;

  for (volatile int rounds = 0; rounds < 128; rounds++)
  {
    BufferStore bufstore(BUFFER_CNT, BUFFER_SZ);
    std::vector<uint8_t*> buffers;

    // force out chained bufferstores
    for (int chain = 0; chain < BS_CHAINS; chain++)
    {
      // deplete each bufferstore
      for (int num = 0; num < BUFFER_CNT; num++) {
        buffers.push_back(bufstore.get_buffer());
      }
      EXPECT(bufstore.available() == 0);
    }

    // release them randomly
    while (!buffers.empty()) {
      int idx = rand() % buffers.size();
      bufstore.release(buffers[idx]);
      buffers.erase(buffers.begin() + idx, buffers.begin() + idx + 1);
    }
    EXPECT(bufstore.available() == BUFFER_CNT * BS_CHAINS);
  }
}
