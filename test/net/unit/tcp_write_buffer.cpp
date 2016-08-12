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
#include <net/tcp/write_buffer.hpp>

using namespace net::tcp;

CASE("Creating a WriteBuffer and operate it")
{
  GIVEN("An WriteBuffer with a buffer_t of 1000 bytes")
  {
    const uint32_t len = 1000;
    WriteBuffer wb { new_shared_buffer(len), len, true };

    EXPECT( wb.length() == len );
    EXPECT( not wb.done() );

    EXPECT( wb.pos() == wb.begin() );
    EXPECT( wb.pos() != wb.end() );

    WHEN("Advanced with 300 bytes")
    {
      bool advanced = wb.advance(300);
      EXPECT( advanced );

      THEN("The buffer is advanced forward, and the length is constant")
      {
        EXPECT( wb.pos() > wb.begin() );
        EXPECT( wb.pos() < wb.end() );

        EXPECT( wb.remaining == 700 );
        EXPECT( wb.offset == 300 );

        EXPECT( wb.length() == len );

        WHEN("Advanced with additional 700 bytes")
        {
          advanced = wb.advance(700);
          EXPECT( advanced );

          THEN("The buffer is at the end, but still not fully done")
          {
            EXPECT( wb.pos() == wb.end() );
            EXPECT( wb.remaining == 0 );

            EXPECT( not wb.done() );

            WHEN("Acknowledged with 1200 bytes")
            {
              auto acked = wb.acknowledge(1200);

              THEN("The buffer is done, but no more than 1000 bytes is acked")
              {
                EXPECT( wb.done() );
                EXPECT( acked == len );
                EXPECT( wb.acknowledged == wb.length() );
              }
            } // < Acknowledge 1200
          } // < THEN
        } // < Advance 700
      } // < THEN
    } // < Advance 300
  } // < GIVEN
} // < CASE #1

