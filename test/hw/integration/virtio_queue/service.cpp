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

/**
   A very superficial test to verify that basic STL is working
   This is useful when we mess with / replace STL implementations

**/


#include <os>

#include <gsl.h>
#include <lest.hpp>
#include <virtio/virtio.hpp>

using namespace std;

const lest::test virtio_tests[] = {
  {


    SCENARIO( "Virtio Queues work" )
    {
      const int queue_size = 256;

      GIVEN( "A virtio queue with 10 tokens" ) {
        Virtio::Queue vq(queue_size, 0, 0);
        enum Direction { IN, OUT };

        EXPECT (vq.num_free() == queue_size);

        WHEN ("You enqueue 10 tokens") {

          for (int i = 0; i < 10; i++) {
            Virtio::Token token {(uint8_t*) malloc(100), 100 };
            vq.enqueue(token , Virtio::Queue::Direction::OUT);
            printf("Free tokens: %i \n", vq.num_free());
          }

          EXPECT (vq.num_free() == (queue_size - 10));

          THEN("You dequeue 10 tokens") {
            uint32_t res;
            for (int i = 0; i < 10; i++)
              vq.dequeue(&res);

            EXPECT (vq.num_free() == 0);
          }
        }

        WHEN ("You insert too many tokens") {

          for (int i = 0; i < queue_size * 3; i++) {
            Virtio::Token token {(uint8_t*) malloc(100), 100 };
            vq.enqueue(token, Virtio::Queue::Direction::OUT);
          }

          THEN ("Queue reports 10 items") {
            EXPECT (vq.num_free() == (queue_size - 10));
          }
        }
      }
    }
  }
};

#define MYINFO(X,...) INFO("Test STL",X,##__VA_ARGS__)

void Service::start(const std::string&)
{

  CHECK( lest::run(virtio_tests, {"-p"}) == 0, "All tests passed" );
  MYINFO("SUCCESS");
}
