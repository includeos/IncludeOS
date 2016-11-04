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

#include <stdlib.h>

// IncludeOS
#include <common.cxx>

const Statman::Size_type NUM_BYTES_GIVEN = 1000;

CASE( "Creating Statman objects" )
{
  GIVEN( "A fixed range of memory and its start position" )
  {
    void* buffer = malloc(NUM_BYTES_GIVEN);
    uintptr_t start = (uintptr_t) buffer;
    Statman::Size_type expected_num_elements = NUM_BYTES_GIVEN / sizeof(Stat);

    WHEN( "Creating Statman" )
    {
      Statman statman_{start, NUM_BYTES_GIVEN};

      THEN( "Statman should have correct number of bytes" )
      {
        Statman::Size_type space_taken = expected_num_elements * sizeof(Stat);
        Statman::Size_type space_not_used = NUM_BYTES_GIVEN - space_taken;

        EXPECT(statman_.num_bytes() == (NUM_BYTES_GIVEN - space_not_used) );
      }

      AND_THEN( "Statman should have room for expected number of Stat-objects" )
      {
        EXPECT(statman_.size() == expected_num_elements);
      }

      AND_THEN( "Statman should contain no Stats" )
      {
        EXPECT(statman_.empty());
        EXPECT(statman_.num_stats() == 0);
        EXPECT_NOT(statman_.full());
      }
    }

    WHEN( "Creating Statman that is given 0 bytes" )
    {
      Statman statman_{start, 0};

      THEN( "Statman is created, but has no room for any Stat-objects" )
      {
        EXPECT(statman_.size() == 0);
        EXPECT(statman_.num_bytes() == 0);
        // Statman is both empty and full (no room for more Stat-objects)
        EXPECT(statman_.empty());
        EXPECT(statman_.full());
      }
    }

    WHEN( "Creating Statman with a negative size an exception is thrown" )
    {
      EXPECT_THROWS_AS((Statman{start, -1}), Stats_exception);
    }

    free(buffer);
  }
}

CASE( "Creating and running through three Stats using Statman iterators begin and last_used" )
{
  GIVEN( "A fixed range of memory and its start position" )
  {
    void* buffer = malloc(NUM_BYTES_GIVEN);
    uintptr_t start = (uintptr_t) buffer;

    WHEN( "Creating Statman" )
    {
      Statman statman_{start, NUM_BYTES_GIVEN};

      EXPECT(statman_.empty());
      EXPECT_NOT(statman_.full());
      EXPECT(statman_.num_stats() == 0);

      THEN( "A Stat can be created" )
      {
        Stat& stat = statman_.create(Stat::UINT32, "net.tcp.dropped");

        EXPECT_NOT(statman_.empty());
        EXPECT(statman_.num_stats() == 1);
        EXPECT_NOT(statman_.full());
        EXPECT(stat.get_uint32() == 0);
        EXPECT_THROWS(stat.get_uint64());
        EXPECT_THROWS(stat.get_float());

        AND_THEN( "The Stat can be incremented in two ways" )
        {
          ++stat;
          stat.get_uint32()++;

          EXPECT(stat.get_uint32() == 2);
          EXPECT_THROWS(stat.get_uint64());
          EXPECT_THROWS(stat.get_float());

          AND_THEN( "Another two Stats can be created" )
          {
            Stat& stat2 = statman_.create(Stat::UINT64, "net.tcp.bytes_transmitted");
            Stat& stat3 = statman_.create(Stat::FLOAT, "net.tcp.average");
            ++stat3;
            stat3.get_float()++;

            EXPECT_NOT(statman_.empty());
            EXPECT(statman_.num_stats() == 3);
            EXPECT_NOT(statman_.full());

            AND_THEN( "The registered Stats can be iterated through and displayed" )
            {
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

    free(buffer);
  }
}

CASE( "Filling Statman with Stats and running through Statman using iterators begin and end" )
{
  GIVEN( "A fixed range of memory and its start position" )
  {
    void* buffer = malloc(NUM_BYTES_GIVEN);
    uintptr_t start = (uintptr_t) buffer;
    Statman::Size_type expected_num_elements = NUM_BYTES_GIVEN / sizeof(Stat);

    WHEN( "Creating Statman" )
    {
      Statman statman_{start, NUM_BYTES_GIVEN};

      EXPECT(statman_.empty());
      EXPECT_NOT(statman_.full());
      EXPECT(statman_.num_stats() == 0);

      AND_WHEN( "Statman is filled with Stats using Statman iterators begin and end" )
      {
        EXPECT(statman_.empty());
        EXPECT(statman_.size() == expected_num_elements);

        int i = 0;

        for (auto it = statman_.begin(); it != statman_.end(); ++it)
        {
          EXPECT(statman_.num_stats() == i);

          if(i % 2 == 0)
          {
            Stat& stat = statman_.create(Stat::UINT32, "net.tcp." + std::to_string(i));
            (stat.get_uint32())++;
            (stat.get_uint32())++;
          }
          else
          {
            Stat& stat = statman_.create(Stat::FLOAT, "net.tcp." + std::to_string(i));
            ++stat;
          }

          i++;
        }

        THEN("Statman is full and the Stats can be displayed using Statman iterators begin and end")
        {
          EXPECT_NOT(statman_.empty());
          EXPECT(statman_.full());
          EXPECT(statman_.num_stats() == statman_.size());

          int j = 0;

          for (auto it = statman_.begin(); it != statman_.end(); ++it)
          {
            Stat& stat = *it;

            EXPECT(stat.name() == "net.tcp." + std::to_string(j));
            EXPECT(stat.index() == j);
            EXPECT_NOT(stat.type() == Stat::UINT64);

            if(j % 2 == 0)
            {
              EXPECT(stat.type() == Stat::UINT32);
              EXPECT(stat.get_uint32() == 2);
              EXPECT_THROWS(stat.get_uint64());
              EXPECT_THROWS(stat.get_float());
            }
            else
            {
              EXPECT(stat.type() == Stat::FLOAT);
              EXPECT(stat.get_float() == 1.0f);
              EXPECT_THROWS(stat.get_uint32());
              EXPECT_THROWS(stat.get_uint64());
            }

            j++;
          }

          AND_WHEN( "A Stat is created when Statman is full an exception is thrown" )
          {
            EXPECT_THROWS_AS(statman_.create(Stat::UINT64, "not.room"), Stats_out_of_memory);
          }
        }
      }
    }

    free(buffer);
  }
}

CASE("A Stat is accessible through index operator")
{
  GIVEN( "A fixed range of memory and its start position" )
  {
    void* buffer = malloc(NUM_BYTES_GIVEN);
    uintptr_t start = (uintptr_t) buffer;

    WHEN("Creating Statman")
    {
      Statman statman_{start, NUM_BYTES_GIVEN};

      AND_WHEN( "Statman is filled with a Stat" )
      {
        Stat& stat = statman_.create(Stat::UINT32, "one.stat");

        THEN("The created Stat can be accessed via the index operator")
        {
          Stat& s = statman_[0];

          EXPECT(s.get_uint32() == stat.get_uint32());
          EXPECT(s.name() == stat.name());
        }
      }
    }

    free(buffer);
  }
}
