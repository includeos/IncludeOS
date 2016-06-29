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

//#define DEBUG // Debug supression

#include <os>
#include <net/buffer_store.hpp>
#include <net/packet.hpp>

using namespace std;
using namespace net;

constexpr size_t bufcount_ {100};
BufferStore bufstore_{ bufcount_,  1500 };


auto create_packet(BufferStore& bufstore, size_t size) {
  // get buffer (as packet + data)
  auto* ptr = (Packet*) bufstore.get_buffer();
  // place packet at front of buffer
  new (ptr) Packet(1500, size,
      [&bufstore] (void* p) { bufstore.release((uint8_t*) p); });
  // regular shared_ptr that calls delete on Packet
  return std::shared_ptr<Packet>(ptr);
}

void Service::start()
{

  INFO("Test net::Packet","Starting tests");

  INFO("Test 1","Naively create, chain and release packets");

  // Create packets, using buffer from the bufstore, and the bufstore's release
  auto packet = create_packet(bufstore_, 1500);
  CHECKSERT(bufstore_.available() == bufcount_ - 1, "Bufcount is now %i", bufcount_ - 1);

  int chain_size = bufcount_;

  // Chain packets
  for (int i = 0; i < chain_size - 1; i++){
    auto chained_packet = create_packet(bufstore_, 1500);
    packet->chain(chained_packet);
    CHECKSERT(bufstore_.available() == bufcount_ - i - 2 , "Bufcount is now %i", bufcount_ - i - 2);
  }

  // Release
  INFO("Test 1","Releaseing packet-chain all at once: Expect bufcount restored");
  packet = 0;
  CHECKSERT(bufstore_.available() == bufcount_ , "Bufcount is now %i", bufcount_);

  INFO("Test 2","Create and chain packets, release one-by-one");

  // Reinitialize the first packet
  packet = create_packet(bufstore_, 1500);
  CHECKSERT(bufstore_.available() == bufcount_ - 1, "Bufcount is now %i", bufcount_ - 1);

  // Chain
  for (int i = 0; i < chain_size - 1; i++){
    auto chained_packet = create_packet(bufstore_, 1500);
    packet->chain(chained_packet);
    CHECKSERT(bufstore_.available() == bufcount_ - i -2, "Bufcount is now %i", bufcount_ - i - 2);
  }

  INFO("Test 2","Releasing packet-chain one-by-one");

  // Release one-by-one
  auto tail = packet;
  size_t i = 0;
  while(tail && i < bufcount_ - 1 ) {
    tail = tail->detach_tail();
    CHECKSERT(bufstore_.available() == i,
              "Bufcount is now %i == %i", i,
              bufstore_.available());
    i++;
  }

  INFO("Test 2","Releasing last packet");
  tail = 0;
  packet = 0;
  CHECKSERT(bufstore_.available() == bufcount_ , "Bufcount is now %i", bufcount_);

  INFO("Tests","SUCCESS");
}
