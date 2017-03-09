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
#include <util/statman.hpp>

const Statman::Size_type NUM_BYTES_GIVEN {1000};

using namespace std;

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
                  EXPECT(s.name() == "net.tcp.dropped"s);
                  EXPECT(s.get_uint32() == 2);
                  EXPECT_THROWS(s.get_uint64());
                  EXPECT_THROWS(s.get_float());
                  EXPECT(s.index() == 0);
                }
                else if (i == 1)
                {
                  EXPECT(s.name() == "net.tcp.bytes_transmitted"s);
                  EXPECT(s.get_uint64() == 0);
                  EXPECT_THROWS(s.get_float());
                  EXPECT(s.index() == 1);
                }
                else
                {
                  EXPECT(s.name() == "net.tcp.average"s);
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

CASE("stats names can only be MAX_NAME_LEN characters long")
{
  uintptr_t buffer = (uintptr_t)malloc(8192);
  Statman statman_{buffer, 8192};
  // ok
  std::string statname1 {"a.stat"};
  Stat& stat1 = statman_.create(Stat::UINT32, statname1);
  // also ok
  const size_t MAX_NAME_LEN {48};
  std::string statname2(MAX_NAME_LEN, 'x');
  Stat& stat2 = statman_.create(Stat::UINT32, statname2);
  int num_stats_before = statman_.num_stats();
  // not ok
  std::string statname3(MAX_NAME_LEN + 1, 'y');
  EXPECT_THROWS(Stat& stat3 = statman_.create(Stat::FLOAT, statname3));
  int num_stats_after = statman_.num_stats();
  EXPECT(num_stats_before == num_stats_after);
  free((void*)buffer);
}

CASE("get(\"name\") returns reference to stat with name, throws if not present")
{
  uintptr_t buffer = (uintptr_t)malloc(8192);
  Statman statman_ {buffer, 8192};
  EXPECT_THROWS(Stat& res1 = statman_.get("some.important.stat"));
  EXPECT_THROWS(Stat& res2 = statman_.get("other.important.stat"));
  EXPECT_THROWS(Stat& res3 = statman_.get("very.important.stat"));
  Stat& stat1 = statman_.create(Stat::UINT32, "some.important.stat");
  Stat& stat2 = statman_.create(Stat::UINT64, "other.important.stat");
  Stat& stat3 = statman_.create(Stat::FLOAT, "very.important.stat");
  ++stat1;
  ++stat2;
  ++stat3;
  EXPECT_NO_THROW(Stat& res1 = statman_.get("some.important.stat"));
  EXPECT_NO_THROW(Stat& res2 = statman_.get("other.important.stat"));
  EXPECT_NO_THROW(Stat& res3 = statman_.get("very.important.stat"));
  EXPECT_THROWS_AS(Stat& res5 = statman_.get("some.missing.stat"), Stats_exception);

  free((void*)buffer);
}

CASE("total_num_bytes() returns total number of bytes the Statman object takes up")
{
  uintptr_t buffer = (uintptr_t)malloc(8192);
  Statman statman_ {buffer, 8192};
  Stat& stat1 = statman_.create(Stat::UINT32, "some.important.stat");
  EXPECT(statman_.total_num_bytes() > 8192);
  free((void*)buffer);
}
