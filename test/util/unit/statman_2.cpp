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

#include <common.cxx>
//#include <util/statman.cpp>
#include <statman>

#include <stdlib.h>

const Statman::Size_type NUM_BYTES_GIVEN = 1000;
const Statman::Size_type NUM_ELEMENTS = 17; // At the moment: 56 bytes per Stat-object (sizeof(Stat) == 56)

CASE( "Creating Statman objects" )
{
  GIVEN( "A fixed range of memory and its start position" )
  {
    printf("MALLOC\n");
    // Memory segmentation fault in case 3 when (if this Case 1 is on top):
    void* buffer = malloc(NUM_BYTES_GIVEN);
    uintptr_t start = (uintptr_t) buffer;

    printf("FREE\n");
    free(buffer);
  }
}

CASE( "Creating and running through three Stats using Statman iterators begin and last_used" )
{
  printf("CASE 2\n");

  GIVEN( "A fixed range of memory and its start position" )
  {
    printf("MALLOC\n");
    void* buffer = malloc(NUM_BYTES_GIVEN);
    uintptr_t start = (uintptr_t) buffer;

    WHEN( "Creating Statman" )
    {
      printf("TWO\n");
      Statman statman_{start, NUM_BYTES_GIVEN};

      EXPECT(statman_.num_stats() == 0);

      THEN( "A Stat can be created" )
      {
        printf("THREE\n");
        Stat& stat = statman_.create(UINT32, "net.tcp.dropped");
      }
    }

    printf("FREE\n");
    free(buffer);
  }
}

CASE( "Filling Statman with Stats and running through Statman using iterators begin and end" )
{
  printf("CASE 3\n");

  GIVEN( "A fixed range of memory and its start position" )
  {
    printf("MALLOC\n");
    void* buffer = malloc(NUM_BYTES_GIVEN);
    uintptr_t start = (uintptr_t) buffer;

    WHEN( "Creating Statman" )
    {
      printf("TWO\n");
      Statman statman_{start, NUM_BYTES_GIVEN};

      EXPECT(statman_.empty());
      EXPECT_NOT(statman_.full());
      EXPECT(statman_.num_stats() == 0);

      AND_WHEN( "Statman is filled with Stats using Statman iterators begin and end" )
      {
        printf("THREE\n");

        EXPECT(statman_.empty());
        EXPECT(statman_.size() == NUM_ELEMENTS);

        // Alt. 2
        for(int i = 0; i < NUM_ELEMENTS; i++)
        {
          EXPECT(statman_.num_stats() == i);
          printf("%d\n", i);
          Stat& stat = statman_.create(FLOAT, "net.tcp." + std::to_string(i));
        }

        // Alt. 1
        /*int i = 0;
        for(auto it = statman_.begin(); it != statman_.end(); ++it)
        {
          EXPECT(statman_.num_stats() == i);
          printf("%d\n", i);
          Stat& stat = statman_.create(FLOAT, "net.tcp." + std::to_string(i));

          i++;
        }*/
      }
    }

    printf("FREE\n");
    free(buffer);
  }
}
