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
#include <net/tcp/write_queue.hpp>

#include <vector>

using namespace net;
using namespace net::tcp;

inline static auto create_write_request(size_t size)
{
  return tcp::construct_buffer(size);
}

CASE("Creating a WriteQueue and operate it")
{
  GIVEN("An empty WriteQueue")
  {
    Write_queue wq;

    EXPECT( wq.empty() );
    EXPECT( wq.size() == 0u );
    EXPECT( wq.current() == 0u );

    EXPECT_THROWS_AS( wq.nxt(), std::out_of_range );
    EXPECT_THROWS_AS( wq.una(), std::out_of_range );

    EXPECT( wq.bytes_total() == 0u );
    EXPECT( wq.bytes_remaining() == 0u );
    EXPECT( wq.bytes_unacknowledged() == 0u );

    WHEN("A WriteRequest is inserted")
    {
      const uint32_t len = 1000;
      auto wr = create_write_request(len);
      wq.push_back(wr);

      THEN("The size is increased, it has remaining requests, nxt() & una() points to the same buffer, but current stays the same")
      {
        EXPECT( not wq.empty() );
        EXPECT( wq.size() == 1u );

        EXPECT( wq.has_remaining_requests() );

        EXPECT( wq.current() == 0u );

        EXPECT( wq.nxt() == wr );
        EXPECT( wq.una() == wr );

        EXPECT( wq.bytes_total() == 1000u );
        EXPECT( wq.bytes_remaining() == 1000u );
        EXPECT( wq.bytes_unacknowledged() == 1000u );

        WHEN("The queue is advanced by 1000 bytes")
        {
          wq.advance(len);

          THEN("The queue has no remaining requests, user callback should be complete, and current should be 1")
          {
            EXPECT( not wq.has_remaining_requests() );
            EXPECT( not wq.empty() );

            EXPECT( wq.current() == 1u );

            EXPECT_THROWS_AS( wq.nxt(), std::out_of_range );
            EXPECT( wq.una() == wr );

            EXPECT( wq.bytes_total() == 1000u );
            EXPECT( wq.bytes_remaining() == 0u );
            EXPECT( wq.bytes_unacknowledged() == 1000u );

            WHEN("The queue is acknowledged with 1000 bytes")
            {
              wq.acknowledge(len);

              THEN("The queue is empty and current is 0")
              {
                EXPECT( wq.empty() );

                EXPECT( wq.current() == 0u );

                EXPECT( wq.bytes_total() == 0u );
                EXPECT( wq.bytes_remaining() == 0u );
                EXPECT( wq.bytes_unacknowledged() == 0u );
              }
            }
          }
        }
      }
      AND_WHEN("The queue is advanced by 200 bytes and then reseted")
      {
        EXPECT( not wq.empty() );

        wq.advance(200);
        wq.reset();

        THEN("The queue is empty, user callback returned 200, and current is 0")
        {
          EXPECT( wq.empty() );

          EXPECT( wq.current() == 0u );

          EXPECT_THROWS_AS( wq.nxt(), std::out_of_range );

          WHEN("Adding additional write request after reset")
          {
            wq.push_back(wr);

            THEN("Everything is fine")
            {
              EXPECT( wq.current() == 0u );
              EXPECT( wq.size() == 1u );
              EXPECT_NO_THROW( wq.nxt() );
            }
          }
        }
      }
    }
    WHEN("Several WriteRequests are inserted")
    {
      const uint32_t N = 5;
      const uint32_t len = 1000;

      std::vector<net::tcp::buffer_t> reqs;
      for(int i = 0; i < N; i++) {
        reqs.emplace_back(create_write_request(len));
        wq.push_back(reqs[i]);
      }

      std::vector<size_t> written = { 0, 0, 0, 0, 0 };
      int i = 0;
      wq.on_write([&written, &i](size_t n) mutable {
        written[i++] = n;
      });

      THEN("The size is increased, and nxt() and una() points to the first element")
      {
        EXPECT( wq.size() == N );

        EXPECT( wq.current() == 0u );

        EXPECT( wq.nxt() == reqs[0] );
        EXPECT( wq.una() == reqs[0] );

        EXPECT( wq.bytes_total() == 5000u );
        EXPECT( wq.bytes_remaining() == 5000u );
        EXPECT( wq.bytes_unacknowledged() == 5000u );

        WHEN("Advanced with 2500 bytes")
        {
          wq.advance(1000);
          wq.advance(1000);
          wq.advance(500);

          THEN("The nxt() is the third element and una() is still the first element, current is 2")
          {
            EXPECT( wq.nxt() == reqs[2] );
            EXPECT( wq.una() == reqs[0] );

            EXPECT( wq.current() == 2u );

            EXPECT( wq.bytes_total() == 5000u );
            EXPECT( wq.bytes_remaining() == 2500u );
            EXPECT( wq.bytes_unacknowledged() == 5000u );

            WHEN("Acknowledged with 2500 bytes")
            {
              wq.acknowledge(2500);

              THEN("nxt() and una() is the third element, size has shrunk to 3, and current is 0")
              {
                EXPECT( wq.nxt() == reqs[2] );
                EXPECT( wq.nxt() == wq.una() );

                EXPECT( wq.size() == 3u );
                EXPECT( wq.current() == 0u );

                EXPECT( wq.has_remaining_requests() );

                EXPECT( wq.bytes_total() == 3000u );
                EXPECT( wq.bytes_remaining() == 2500u );
                EXPECT( wq.bytes_unacknowledged() == 2500u );

                WHEN("The queue is reset")
                {
                  wq.reset();

                  THEN("The queue is empty and on write has returned correct values on the fully/half completed ones [1000, 1000, 500]")
                  {
                    EXPECT( wq.empty() );
                    EXPECT( wq.size() == 0u );
                    EXPECT( not wq.has_remaining_requests() );
                    EXPECT( wq.current() == 0u );

                    EXPECT_THROWS_AS( wq.nxt(), std::out_of_range );
                    EXPECT_THROWS_AS( wq.una(), std::out_of_range );

                    EXPECT( written[0] == 1000u );
                    EXPECT( written[1] == 1000u );
                    EXPECT( written[2] == 500u );
                    EXPECT( written[3] == 0u );
                    EXPECT( written[4] == 0u );

                    EXPECT( wq.bytes_total() == 0u );
                    EXPECT( wq.bytes_remaining() == 0u );
                    EXPECT( wq.bytes_unacknowledged() == 0u );
                  }
                }
              }
            }
          }
        }
      }
    }
  }
};
