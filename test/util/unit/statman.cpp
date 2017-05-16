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
    auto* buffer = new char[NUM_BYTES_GIVEN];
    uintptr_t start = (uintptr_t) buffer;
    Statman::Size_type expected_num_elements = NUM_BYTES_GIVEN / sizeof(Stat);

    WHEN("Creating Statman")
    {
      Statman statman_{start, NUM_BYTES_GIVEN};

      EXPECT(statman_.capacity() == expected_num_elements);
      EXPECT(statman_.size() == 0);
      EXPECT(statman_.num_bytes() == 0);

      EXPECT(statman_.empty());
      EXPECT(!statman_.full());
    }

    WHEN( "Creating Statman that is given 0 bytes" )
    {
      Statman statman_{start, 0};

      EXPECT(statman_.capacity() == 0);
      EXPECT(statman_.size() == 0);
      EXPECT(statman_.num_bytes() == 0);
      EXPECT(statman_.empty());
      EXPECT(statman_.full());

      EXPECT_THROWS(Stat& stat = statman_.create(Stat::UINT32, "some.new.stat"));
      EXPECT_THROWS(statman_[0]);
    }

    WHEN( "Creating Statman with a negative size an exception is thrown" )
    {
      EXPECT_THROWS_AS((Statman{start, -1}), Stats_exception);
    }

    delete[] buffer;
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
      EXPECT(statman_.size() == 0);

      THEN( "A Stat can be created" )
      {
        Stat& stat = statman_.create(Stat::UINT32, "net.tcp.dropped");
        EXPECT(stat.get_uint32() == 0);

        EXPECT_NOT(statman_.empty());
        EXPECT_NOT(statman_.full());
        EXPECT(statman_.size() == 1);
        EXPECT_THROWS(stat.get_uint64());
        EXPECT_THROWS(stat.get_float());
        EXPECT(stat.get_uint32() == 0);

        AND_THEN( "The Stat can be incremented in two ways" )
        {
          ++stat;
          EXPECT(stat.get_uint32() == 1);
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
            EXPECT_NOT(statman_.full());
            EXPECT(statman_.size() == 3);

            AND_THEN( "The registered Stats can be iterated through and displayed" )
            {
              int i = 0;

              for (auto it = statman_.begin(); it != statman_.end(); ++it)
              {
                Stat& s = *it;

                if (i == 0)
                {
                  EXPECT(s.name() == "net.tcp.dropped"s);
                  EXPECT(s.get_uint32() == 2);
                  EXPECT_THROWS(s.get_uint64());
                  EXPECT_THROWS(s.get_float());
                }
                else if (i == 1)
                {
                  EXPECT(s.name() == "net.tcp.bytes_transmitted"s);
                  EXPECT(s.get_uint64() == 0);
                  EXPECT_THROWS(s.get_float());
                }
                else if (i == 2)
                {
                  EXPECT(s.name() == "net.tcp.average"s);
                  EXPECT(s.get_float() == 2.0f);
                  EXPECT_THROWS(s.get_uint32());
                  EXPECT_THROWS(s.get_uint64());
                }
                else {
                  EXPECT(i < 3);
                }

                i++;
              }

              EXPECT(i == 3);

              // note: if you move this, it might try to delete
              // the stats before running the above
              AND_THEN("Delete the stats")
              {
                EXPECT(statman_.size() == 3);
                statman_.free(&statman_[0]);
                EXPECT(statman_.size() == 2);
                statman_.free(&statman_[1]);
                EXPECT(statman_.size() == 1);
                statman_.free(&statman_[2]);
                EXPECT(statman_.size() == 0);
              }
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
      EXPECT(statman_.size() == 0);

      AND_WHEN( "Statman is filled with Stats" )
      {
        EXPECT(statman_.empty());
        EXPECT(statman_.capacity() == expected_num_elements);

        // fill it to the brim
        for (int i = 0; i < statman_.capacity(); i++)
        {
          EXPECT(statman_.size() == i);

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
        }

        THEN("Statman is full and the Stats can be displayed using Statman iterators begin and end")
        {
          EXPECT_NOT(statman_.empty());
          EXPECT(statman_.full());
          EXPECT(statman_.size() == statman_.capacity());

          int j = 0;

          for (auto it = statman_.begin(); it != statman_.end(); ++it)
          {
            Stat& stat = *it;

            EXPECT(stat.name() == "net.tcp." + std::to_string(j));
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
    char buffer[NUM_BYTES_GIVEN];
    uintptr_t start = (uintptr_t) buffer;

    WHEN("Creating Stat with Statman")
    {
      Statman statman_{start, NUM_BYTES_GIVEN};
      Stat& stat = statman_.create(Stat::UINT32, "one.stat");

      THEN("The created Stat can be accessed via the index operator")
      {
        Stat& s = statman_[0];

        EXPECT(s.get_uint32() == stat.get_uint32());
        EXPECT(s.name() == std::string("one.stat"));
      }
    }
  }
}

CASE("stats names can only be MAX_NAME_LEN characters long")
{
  char buffer[8192];
  Statman statman_((uintptr_t) buffer, sizeof(buffer));
  // ok
  std::string statname1 {"a.stat"};
  Stat& stat1 = statman_.create(Stat::UINT32, statname1);
  // also ok
  std::string statname2(Stat::MAX_NAME_LEN, 'x');
  Stat& stat2 = statman_.create(Stat::UINT32, statname2);
  int size_before = statman_.size();
  // not ok
  std::string statname3(Stat::MAX_NAME_LEN + 1, 'y');
  EXPECT_THROWS(statman_.create(Stat::FLOAT, statname3));
  int size_after = statman_.size();
  EXPECT(size_before == size_after);
}

CASE("get(addr) returns reference to stat, throws if not present")
{
  char buffer[8192];
  Statman statman_((uintptr_t) buffer, sizeof(buffer));

  EXPECT_THROWS(statman_.get((void*) 0));
  EXPECT_THROWS(statman_.get(statman_.begin()));
  Stat& stat1 = statman_.create(Stat::UINT32, "some.important.stat");
  Stat& stat2 = statman_.create(Stat::UINT64, "other.important.stat");
  Stat& stat3 = statman_.create(Stat::FLOAT, "very.important.stat");
  ++stat1;
  ++stat2;
  ++stat3;
  EXPECT_NO_THROW(statman_.get(&stat1));
  EXPECT_NO_THROW(statman_.get(&stat2));
  EXPECT_NO_THROW(statman_.get(&stat3));
  EXPECT_THROWS(statman_.get(statman_.end()));

  // Can't create stats with empty name
  EXPECT_THROWS_AS(statman_.create(Stat::UINT32, ""), Stats_exception);
}
