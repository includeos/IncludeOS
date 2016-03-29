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
BufferStore bufstore_{ bufcount_,  1500, 10 };

void Service::start()
{

  INFO("Test net::Packet","Starting tests");

  INFO("Test 1","Naively create, chain and release packets");

  // Create a release delegate - i.e. the thing in charge of releasing packets
  auto release = BufferStore::release_del::from
    <BufferStore, &BufferStore::release_offset_buffer>(bufstore_);

  // Create packets, using buffer from the bufstore, and the bufstore's release
  auto packet = std::make_shared<Packet>(bufstore_.get_offset_buffer(),
                                         bufstore_.offset_bufsize(), 1500, release);

  CHECK(bufstore_.buffers_available() == bufcount_ - 1, "Bufcount is now %i", bufcount_ -1);

  int chain_size = bufcount_;

  // Chain packets
  for (int i = 0; i < chain_size - 1; i++){
    auto chained_packet = std::make_shared<Packet>(bufstore_.get_offset_buffer(),
                                                   bufstore_.offset_bufsize(), 1500, release);
    packet->chain(chained_packet);
    CHECK(bufstore_.buffers_available() == bufcount_ - i - 2 , "Bufcount is now %i", bufcount_ - i -2);
  }


  // Release
  INFO("Test 1","Releaseing packet-chain all at once: Expect bufcount restored");
  packet = 0;
  CHECK(bufstore_.buffers_available() == bufcount_ , "Bufcount is now %i", bufcount_);

  INFO("Test 2","Create and chain packets, release one-by-one");

  // Reinitialize the first packet
  packet = std::make_shared<Packet>(bufstore_.get_offset_buffer(),
                                         bufstore_.offset_bufsize(), 1500, release);

  CHECK(bufstore_.buffers_available() == bufcount_ - 1, "Bufcount is now %i", bufcount_ -1);

  // Chain
  for (int i = 0; i < chain_size - 1; i++){
    auto chained_packet = std::make_shared<Packet>(bufstore_.get_offset_buffer(),
                                                   bufstore_.offset_bufsize(), 1500, release);
    packet->chain(chained_packet);
    CHECK(bufstore_.buffers_available() == bufcount_ - i -2, "Bufcount is now %i", bufcount_ - i -2);
  }

  INFO("Test 2","Releaseing packet-chain one-by-one");

  // Release one-by-one
  auto tail = packet;
  size_t i = 0;
  while(tail && i < bufcount_ - 1 ) {
    tail = tail->detach_tail();
    CHECK(bufstore_.buffers_available() == i,
          "Bufcount is now %i == %i", i,
          bufstore_.buffers_available());
    i++;
  }

  INFO("Test 2","Releaseing last packet");
  tail = 0;
  packet = 0;
  CHECK(bufstore_.buffers_available() == bufcount_ , "Bufcount is now %i", bufcount_);



}
