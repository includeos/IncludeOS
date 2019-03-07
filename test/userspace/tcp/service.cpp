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

#include <os>
#include <statman>
#include <hw/async_device.hpp>
#include <net/inet>
#include <net/interfaces>
static constexpr bool debug = false;
static const size_t CHUNK_SIZE = 1024 * 1024;
static const size_t NUM_CHUNKS = 2048;
static const uint16_t      MTU = 6000;
static std::unique_ptr<hw::Async_device<UserNet>> dev1 = nullptr;
static std::unique_ptr<hw::Async_device<UserNet>> dev2 = nullptr;

using namespace std::chrono;
static milliseconds time_start;

static inline auto now() {
  return duration_cast< milliseconds >(system_clock::now().time_since_epoch());
}

void Service::start()
{
  dev1 = std::make_unique<hw::Async_device<UserNet>>(UserNet::create(MTU));
  dev2 = std::make_unique<hw::Async_device<UserNet>>(UserNet::create(MTU));
  dev1->connect(*dev2);
  dev2->connect(*dev1);

  // Create IP stacks on top of the nic's and configure them
  auto& inet_server = net::Interfaces::get(0);
  auto& inet_client = net::Interfaces::get(1);
  inet_server.network_config({10,0,0,42}, {255,255,255,0}, {10,0,0,1});
  inet_client.network_config({10,0,0,43}, {255,255,255,0}, {10,0,0,1});


  // Set up a TCP server on port 80
  auto& server = inet_server.tcp().listen(80);
  // the shared buffer
  auto buf = net::tcp::construct_buffer(CHUNK_SIZE);

  // Add a TCP connection handler
  server.on_connect(
  [] (net::tcp::Connection_ptr conn)
  {
    conn->on_read(CHUNK_SIZE, [conn] (auto buf) {
        static size_t   count_bytes = 0;

        assert(buf->size() <= CHUNK_SIZE);
        count_bytes += buf->size();

        if constexpr (debug) {
        static uint32_t count_chunks = 0;
        printf("Received chunk %u (%zu bytes) for a total of %zu / %zu kB\n",
               ++count_chunks, buf->size(), count_bytes / 1024, NUM_CHUNKS * CHUNK_SIZE / 1024);
        }
        if (count_bytes >= NUM_CHUNKS * CHUNK_SIZE)
        {
          auto timediff = now() - time_start;
          assert(count_bytes == NUM_CHUNKS * CHUNK_SIZE);

          double time_sec = timediff.count()/1000.0;
          double mbps = ((count_bytes * 8) / (1024.0 * 1024.0)) / time_sec;

          printf("Server received %zu Mb in %f sec. - %f Mbps \n",
                 count_bytes / (1024 * 1024), time_sec,  mbps);

          for (const auto& stat : Statman::get())
          {
            printf("-> %s: %s\n", stat.name(), stat.to_string().c_str());
          }
          os::shutdown();
        }

      });
  });

  printf("*** Userspace TCP benchmark started ***\n");

  printf("Measuring memory <-> memory bandwidth...\n");
  inet_client.tcp().connect({net::ip4::Addr{"10.0.0.42"}, 80},
    [buf](auto conn)
    {
      if constexpr (debug) {
          printf("Connected\n");
      }
      assert(conn != nullptr);
      time_start = now();
      for (size_t i = 0; i < NUM_CHUNKS; i++) {
        conn->write(buf);
      }
    });
}
