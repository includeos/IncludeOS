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

using namespace std;

CASE( "Creating Statman objects" )
{
  GIVEN( "A fixed range of memory and its start position" )
  {
    WHEN("Creating Statman")
    {
      Statman statman_;
      EXPECT(statman_.size() == 1);
      EXPECT(statman_.num_bytes() == (sizeof(Statman) + sizeof(Stat)));
      EXPECT(not statman_.empty());
    }
  }
}

CASE( "Creating and running through three Stats using Statman iterators begin and last_used" )
{
  Statman statman_;
  EXPECT(not statman_.empty());
  EXPECT(statman_.size() == 1);

  // create single stat
  Stat& stat = statman_.create(Stat::UINT32, "net.tcp.dropped");
  EXPECT(stat.get_uint32() == 0);

  // verify container
  EXPECT_NOT(statman_.empty());
  EXPECT(statman_.size() == 2);
  EXPECT_THROWS(stat.get_uint64());
  EXPECT_THROWS(stat.get_float());
  EXPECT(stat.get_uint32() == 0);

  // increment stat
  ++stat;
  EXPECT(stat.get_uint32() == 1);
  stat.get_uint32()++;
  EXPECT(stat.get_uint32() == 2);

  // interpret wrongly throws
  EXPECT_THROWS(stat.get_uint64());
  EXPECT_THROWS(stat.get_float());

  // create more stats
  Stat& stat2 = statman_.create(Stat::UINT64, "net.tcp.bytes_transmitted");
  Stat& stat3 = statman_.create(Stat::FLOAT, "net.tcp.average");
  ++stat3;
  stat3.get_float()++;

  EXPECT_NOT(statman_.empty());
  EXPECT(statman_.size() == 4);

  // test stat iteration
  int i = 0;
  for (auto it = statman_.begin(); it != statman_.end(); ++it)
  {
    const Stat& s = *it;

    if (i == 1)
    {
      EXPECT(s.name() == "net.tcp.dropped"s);
      EXPECT(s.get_uint32() == 2);
      EXPECT_THROWS(s.get_uint64());
      EXPECT_THROWS(s.get_float());
    }
    else if (i == 2)
    {
      EXPECT(s.name() == "net.tcp.bytes_transmitted"s);
      EXPECT(s.get_uint64() == 0);
      EXPECT_THROWS(s.get_float());
    }
    else if (i == 3)
    {
      EXPECT(s.name() == "net.tcp.average"s);
      EXPECT(s.get_float() == 2.0f);
      EXPECT_THROWS(s.get_uint32());
      EXPECT_THROWS(s.get_uint64());
    }
    else {
      EXPECT(i == 0);
    }

    i++;
  }
  EXPECT(i == 4);

  // free 3 stats we just created (leaving 1)
  EXPECT(statman_.size() == 4);
  statman_.free(&statman_[1]);
  EXPECT(statman_.size() == 3);
  statman_.free(&statman_[2]);
  EXPECT(statman_.size() == 2);
  statman_.free(&statman_[3]);
  EXPECT(statman_.size() == 1);

  // clear out the whole container (not a good idea!)
  EXPECT(!statman_.empty());
  statman_.clear();
  EXPECT(statman_.size() == 1);
}

CASE( "Filling Statman with Stats and running through Statman using iterators begin and end" )
{
  GIVEN( "A fixed range of memory and its start position" )
  {
    WHEN( "Creating Statman" )
    {
      Statman statman_;
      EXPECT(not statman_.empty());

      AND_WHEN( "Statman is filled with Stats" )
      {
        // fill it to the brim
        for (int i = 1; i < 101; i++)
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
          EXPECT(statman_.size() == 101);

          int j = 0;
          for (auto it = statman_.begin(); it != statman_.end(); ++it)
          {
            const Stat& stat = *it;

            if (j > 0) {
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
            }

            j++;
          }
        }
      }
    }
  }
}

CASE("A Stat is accessible through index operator")
{
  GIVEN( "A fixed range of memory and its start position" )
  {
    WHEN("Creating Stat with Statman")
    {
      Statman statman_;
      Stat& stat = statman_.create(Stat::UINT32, "one.stat");

      THEN("The created Stat can be accessed via the index operator")
      {
        Stat& s = statman_[1];

        EXPECT(s.get_uint32() == stat.get_uint32());
        EXPECT(s.name() == std::string("one.stat"));
      }
    }
  }
}

CASE("stats names can only be MAX_NAME_LEN characters long")
{
  Statman statman_;
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
  Statman statman_;
  EXPECT_THROWS(statman_.get((Stat*) 0));
  EXPECT_THROWS(statman_.get(&statman_[1]));
  Stat& stat1 = statman_.create(Stat::UINT32, "some.important.stat");
  Stat& stat2 = statman_.create(Stat::UINT64, "other.important.stat");
  Stat& stat3 = statman_.create(Stat::FLOAT, "very.important.stat");
  ++stat1;
  ++stat2;
  ++stat3;
  EXPECT_NO_THROW(statman_.get(&stat1));
  EXPECT_NO_THROW(statman_.get(&stat2));
  EXPECT_NO_THROW(statman_.get(&stat3));
  EXPECT_THROWS(statman_.get(&*statman_.end()));

  // Can't create stats with empty name
  EXPECT_THROWS_AS(statman_.create(Stat::UINT32, ""), Stats_exception);
}

CASE("Various stat to_string()")
{
  Statman statman_;
  Stat& stat1 = statman_.create(Stat::UINT32, "some.important.stat");
  Stat& stat2 = statman_.create(Stat::UINT64, "other.important.stat");
  Stat& stat3 = statman_.create(Stat::FLOAT, "very.important.stat");
  ++stat1;
  ++stat2;
  ++stat3;
  
  EXPECT(stat1.to_string() == std::to_string(1u));
  EXPECT(stat2.to_string() == std::to_string(1ul));
  EXPECT(stat3.to_string() == std::to_string(1.0f));
}
