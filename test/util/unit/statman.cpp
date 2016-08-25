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
// Alt. static char array 1000 plasser
// memseter etter given på nytt og på nytt - fungerer

CASE( "Creating Statman objects" )
{
  GIVEN( "A fixed range of memory and its start position" )
  {
    printf("CASE 1\n");
    printf("ONE\n");

    // Memory segmentation fault in case 3 when (if this Case 1 is on top):
    void* buffer = malloc(NUM_BYTES_GIVEN);
    uintptr_t start = (uintptr_t) buffer;
    free(buffer);

    /*WHEN( "Creating Statman" )
    {
      printf("TWO-1\n");

      Statman statman_{start, NUM_BYTES_GIVEN};

      THEN( "Statman should have correct number of bytes, start- and end-position" )
      {
        printf("THREE-1\n");

        Statman::Size_type space_taken = NUM_ELEMENTS * sizeof(Stat);
        Statman::Size_type space_not_used = NUM_BYTES_GIVEN - space_taken;

        EXPECT(statman_.num_bytes() == (NUM_BYTES_GIVEN - space_not_used) );
        EXPECT(statman_.addr_start() == start);
        EXPECT(statman_.addr_end() == (statman_.addr_start() + space_taken));
      }

      AND_THEN( "Statman should have room for NUM_ELEMENTS Stat-objects" )
      {
        printf("THREE-2\n");

        EXPECT(statman_.size() == NUM_ELEMENTS);
      }

      AND_THEN( "Statman should contain no Stats" )
      {
        printf("THREE-3\n");

        EXPECT(statman_.empty());
        EXPECT(statman_.num_stats() == 0);
        EXPECT_NOT(statman_.full());
      }
    }

    WHEN( "Creating Statman that is given 0 bytes" )
    {
      printf("TWO-2\n");

      Statman statman_{start, 0};

      THEN( "Statman is created, but has no room for any Stat-objects" )
      {

        printf("2-THREE\n");

        EXPECT(statman_.size() == 0);
        EXPECT(statman_.num_bytes() == 0);
        EXPECT(statman_.addr_end() == start);
        // Statman is both empty and full (no room for more Stat-objects)
        EXPECT(statman_.empty());
        EXPECT(statman_.full());
      }
    }

    WHEN( "Creating Statman with a negative size an exception is thrown" )
    {
      printf("TWO-3\n");

      EXPECT_THROWS_AS((Statman{start, -1}), Stats_exception);
    }

    printf("GIVEN-FINISHED\n");

    free(buffer);*/
  }
}

CASE( "Creating and running through three Stats using Statman iterators begin and last_used" )
{
  printf("CASE 2\n");

  GIVEN( "A fixed range of memory and its start position" )
  {
    printf("ONE\n");

    void* buffer = malloc(NUM_BYTES_GIVEN);
    uintptr_t start = (uintptr_t) buffer;

    WHEN( "Creating Statman" )
    {
      printf("TWO\n");

      Statman statman_{start, NUM_BYTES_GIVEN};

      EXPECT(statman_.empty());
      EXPECT_NOT(statman_.full());
      EXPECT(statman_.num_stats() == 0);

      THEN( "A Stat can be created" )
      {
        printf("THREE\n");

        Stat& stat = statman_.create(UINT32, "net.tcp.dropped");

        EXPECT_NOT(statman_.empty());
        EXPECT(statman_.num_stats() == 1);
        EXPECT_NOT(statman_.full());
        EXPECT(stat.get_uint32() == 0);
        EXPECT_THROWS(stat.get_uint64());
        EXPECT_THROWS(stat.get_float());

        AND_THEN( "The Stat can be incremented in two ways" )
        {
          printf("FOUR\n");

          ++stat;
          stat.get_uint32()++;

          EXPECT(stat.get_uint32() == 2);
          EXPECT_THROWS(stat.get_uint64());
          EXPECT_THROWS(stat.get_float());

          AND_THEN( "Another two Stats can be created" )
          {
            printf("FIVE\n");

            Stat& stat2 = statman_.create(UINT64, "net.tcp.bytes_transmitted");
            Stat& stat3 = statman_.create(FLOAT, "net.tcp.average");
            ++stat3;
            stat3.get_float()++;

            EXPECT_NOT(statman_.empty());
            EXPECT(statman_.num_stats() == 3);
            EXPECT_NOT(statman_.full());

            AND_THEN( "The registered Stats can be iterated through and displayed" )
            {
              printf("SIX\n");

              int i = 0;

              for (auto it = statman_.begin(); it != statman_.last_used(); ++it)
              {
                Stat& s = *it;

                if (i == 0)
                {
                  EXPECT(s.name() == "net.tcp.dropped");
                  EXPECT(s.get_uint32() == 2);
                  EXPECT_THROWS(s.get_uint64());
                  EXPECT_THROWS(s.get_float());
                  EXPECT(s.index() == 0);
                }
                else if (i == 1)
                {
                  EXPECT(s.name() == "net.tcp.bytes_transmitted");
                  EXPECT(s.get_uint64() == 0);
                  EXPECT_THROWS(s.get_float());
                  EXPECT(s.index() == 1);
                }
                else
                {
                  EXPECT(s.name() == "net.tcp.average");
                  EXPECT(s.get_float() == 2.0f);
                  EXPECT_THROWS(s.get_uint32());
                  EXPECT_THROWS(s.get_uint64());
                }

                i++;
              }

              EXPECT(i == 3);
            }
          }
        }
      }
    }

    printf("GIVEN FINISHED\n");

    free(buffer);
  }
}

CASE( "Filling Statman with Stats and running through Statman using iterators begin and end" )
{
  printf("CASE 3\n");

  GIVEN( "A fixed range of memory and its start position" )
  {
    printf("ONE\n");

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
        for (int i = 0; i < NUM_ELEMENTS; i++)
        {
          printf("%d\n", i);

          EXPECT(statman_.num_stats() == i);

          if(i % 2 == 0)
          {
            Stat& stat = statman_.create(UINT32, "net.tcp." + std::to_string(i));
            stat.get_uint32()++;
            stat.get_uint32()++;
          }
          else
          {
            Stat& stat = statman_.create(FLOAT, "net.tcp." + std::to_string(i));
            ++stat;
          }
        }

        /* Alt. 1
        for (auto it = statman_.begin(); it != statman_.end(); ++it)
        {
          printf("%d\n", i);

          EXPECT(statman_.num_stats() == i);

          if(i % 2 == 0)
          {
            Stat& stat = statman_.create(UINT32, "net.tcp." + std::to_string(i));
            stat.get_uint32()++;
            stat.get_uint32()++;
          }
          else
          {
            Stat& stat = statman_.create(FLOAT, "net.tcp." + std::to_string(i));
            ++stat;
          }

          i++;
        }*/

        /*THEN("Statman is full and the Stats can be displayed using Statman iterators begin and end")
        {
          printf("FOUR\n");

          EXPECT_NOT(statman_.empty());
          EXPECT(statman_.full());
          EXPECT(statman_.num_stats() == statman_.size());

          int j = 0;

          for (auto it = statman_.begin(); it != statman_.end(); ++it)
          {
            Stat& stat = *it;

            EXPECT(stat.name() == "net.tcp." + std::to_string(j));
            EXPECT(stat.index() == j);
            EXPECT_NOT(stat.type() == UINT64);

            if(j % 2 == 0)
            {
              EXPECT(stat.type() == UINT32);
              EXPECT(stat.get_uint32() == 2);
              EXPECT_THROWS(stat.get_uint64());
              EXPECT_THROWS(stat.get_float());
            }
            else
            {
              EXPECT(stat.type() == FLOAT);
              EXPECT(stat.get_float() == 1.0f);
              EXPECT_THROWS(stat.get_uint32());
              EXPECT_THROWS(stat.get_uint64());
            }

            j++;
          }

          AND_WHEN( "A Stat is created when Statman is full an exception is thrown" )
          {
            printf("FIVE\n");

            EXPECT_THROWS_AS(statman_.create(UINT64, "not.room"), Stats_out_of_memory);
          }
        }*/
      }
    }

    printf("GIVEN FINISHED\n");

    free(buffer);
  }
}

// Teste:
// Statman-klassen:
//  Iteratorer (cbegin, cend)
// Stat-klassen:
//  ++

/*
      stat_type type = UINT32;

      for (int i = 0; i < num_stats_in_span; i++) {
        Stat& s = statman_.create(type, "net.tcp." + std::to_string(i));
        EXPECT(s.name() == "net.tcp." + std::to_string(i));
        EXPECT(s.get_uint32() == 0);

        printf("Name stat: %s\n", s.name().c_str());

        ++s;

        EXPECT(s.get_uint32() == 1);

        printf("Stat value: %u\n", s.get_uint32());
      }

      EXPECT_THROWS_AS(statman_.create(type, "not.room"), Stats_out_of_memory);

      // Testing last_used
      auto it_last_used = statman_.last_used();

      // Iterate until last used element:
      for (auto it = statman_.begin(); it != it_last_used; ++it) {
        Stat& stat = (*it);
        printf("Iterate stat name: %s\n", stat.name().c_str());
      }

      free(buffer);
*/
      /*
      stat_type type = UINT64;

      Stat stat = statman_.create(type, "net.tcp.dropped");

      EXPECT(stat.type() == UINT64);
      EXPECT_NOT(stat.type() == UINT32);
      EXPECT_NOT(stat.type() == FLOAT);

      EXPECT(stat.get_uint64() == 0);

      ++stat;

      //EXPECT(stat.get_uint64() == 0);
      //EXPECT(stat.get_uint64() == 1);

      EXPECT(stat.get_uint64() == 1);
      EXPECT_NOT(stat.get_uint64() == 0);
      EXPECT_NOT(stat.get_uint32() == 1);
      EXPECT_NOT(stat.get_float() == 1);

      ++stat;
      ++stat;

      EXPECT(stat.get_uint64() == 3);
      EXPECT_NOT(stat.get_uint64() == 0);
      EXPECT_NOT(stat.get_uint32() == 1);
      EXPECT_NOT(stat.get_float() == 1);

      EXPECT(stat.name() == "net.tcp.dropped");
      EXPECT(stat.name() == "net.");
      EXPECT_NOT(stat.name() == "net.tcp.dropped");*/

/*
  TODO

  Teste rule of five Statman (ikke test-case, men sjekk hva som går
  i kompileringen - at det blir riktige regler)

  Postfix ++ på Stat-objekt ?


 */
