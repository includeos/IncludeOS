// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2016-2017 Oslo and Akershus University College of Applied Sciences
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
#include <virtio/virtio.hpp>

CASE("Virtio Queue enqueue")
{
  Virtio::Queue q("Test queue", 4096, 0, 0x1000);
  
  EXPECT(q.size() == 4096);

  uint8_t  value = 4;
  uint8_t  buffer[16];

  Virtio::Token token1 {{&value, sizeof(value)}, Virtio::Token::IN };
  Virtio::Token token2 {{buffer, sizeof(buffer)}, Virtio::Token::IN };

  std::array<Virtio::Token, 2> tokens {{ token1, token2 }};
  q.enqueue(tokens);
  // update avail idx
  q.kick();
}

CASE("Virtio Queue interrupts")
{
  Virtio::Queue q("Test queue", 4096, 0, 0x1000);
  q.enable_interrupts();
  EXPECT(q.interrupts_enabled());
  q.disable_interrupts();
  EXPECT(!q.interrupts_enabled());
}

CASE("Virtio Queue dequeue")
{
  Virtio::Queue q("Test queue", 4096, 0, 0x1000);
  
  EXPECT(q.size() == 4096);

  auto res = q.dequeue();
  EXPECT(res.size() == 0);
  EXPECT(res.data() == nullptr);
}
