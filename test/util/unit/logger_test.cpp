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

#include "../logger.hpp"
#include "../../test/lest/include/lest/lest.hpp"

const lest::test test_logger[] =
{

  CASE("Creating a Logger")
  {
    GIVEN("A gsl::span<char>")
    {
      const int N = 100;
      char buffer[N] = { 0 };
      //char buffer[N];
      gsl::span<char> log{buffer, N};
      Logger logger{log};

      WHEN("A Logger is created")
      {
        Logger logger{log};

        THEN("Logger returns the correct size")
        {
          EXPECT( logger.size() == N );
        }
        AND_THEN("Logger generates no entries")
        {
          auto entries = logger.entries();
          EXPECT( entries.empty() );
        }
      }
    }
  },
  CASE("Adding one entry")
  {
    GIVEN("An initialized Logger")
    {
      const int N = 100;
      char buffer[N] = { 0 };
      gsl::span<char> log{buffer, N};
      Logger logger{log};

      WHEN("An entry is logged")
      {
        const std::string entry{"test"};
        logger.log(entry);

        THEN("The logger have one entry which is the one logged")
        {
          auto entries = logger.entries();

          EXPECT( entries.size() == 1 );
          EXPECT( entries[0] == entry );

          WHEN("The logger is flushed")
          {
            logger.flush();

            THEN("The logger has no entries")
            {
              auto entries = logger.entries();

              EXPECT( entries.empty() );
            }
          }
        }
      }
      WHEN("An empty entry is logged")
      {
        logger.log("");

        THEN("The entry is never logged")
        {
          auto entries = logger.entries();

          EXPECT( entries.empty() );
        }
      }
    }
  },
  CASE("Adding several entries")
  {
    GIVEN("An initialized Logger")
    {
      const int N = 100;
      char buffer[N] = { 0 };
      gsl::span<char> log{buffer, N};
      Logger logger{log};

      WHEN("3 entries are logged")
      {
        const std::string entry1{"First"};
        const std::string entry2{"Second"};
        const std::string entry3{"Third"};

        logger.log(entry1);
        logger.log(entry2);
        logger.log(entry3);

        THEN("The logger has 3 entries in the correct order")
        {
          auto entries = logger.entries();

          EXPECT( entries.size() == 3 );

          EXPECT( entries[0] == entry1 );
          EXPECT( entries[1] == entry2 );
          EXPECT( entries[2] == entry3 );

          WHEN("Asking for 1 result")
          {
            entries = logger.entries(1);

            THEN("Only the last (latest) entry is returned")
            {
              EXPECT( entries.size() == 1 );
              EXPECT( entries[0] == entry3 );
            }
          }
        }
      }
    }
  },
  CASE("Adding entries that wraps around")
  {
    GIVEN("An initialized Logger with two entries")
    {
      const int N = 14;
      char buffer[N] = { 0 };
      gsl::span<char> log{buffer, N};
      Logger logger{log};

      const std::string entry1{"First"}; // 5 + 1
      const std::string entry2{"Second"}; // 6 + 1

      logger.log(entry1);
      logger.log(entry2);

      WHEN("The third entry logged is smaller or equal first entry")
      {
        const std::string entry3{"Third"}; // 5 + 1
        logger.log(entry3);

        THEN("The oldest (first) entry will be overwritten, and the second and third entry will be returned")
        {
          auto entries = logger.entries();

          EXPECT( entries.size() == 2 );
          EXPECT( entries[0] == entry2 );
          EXPECT( entries[1] == entry3 );
        }
      }
      WHEN("The third entry logged is bigger than the first entry")
      {
        const std::string entry3{"A big third"}; // 11 + 1
        logger.log(entry3);

        THEN("Both the current entries will be overwritten, and the third will be returned")
        {
          auto entries = logger.entries();

          EXPECT( entries.size() == 1 );
          EXPECT( entries[0] == entry3 );
        }
      }
      WHEN("The third entry logged is bigger than the size of the buffer")
      {
        const std::string entry3{"This one is too big"}; // 19 + 1
        logger.log(entry3);

        THEN("Only the last part of the string will be in the entry")
        {
          auto offset = entry3.size() + 1 - logger.size(); // +1 to account for padding
          const std::string substr{entry3.begin() + offset, entry3.end()};

          auto entries = logger.entries();

          EXPECT( entries.size() == 1 );
          EXPECT( entries[0] == substr );
        }
      }
      WHEN("An empty entry is logged")
      {
        logger.log("");

        THEN("The entry is never logged")
        {
          auto entries = logger.entries();

          EXPECT( entries.size() == 2 );
        }
      }
    }
  },
  CASE("Adding a horde of log entries")
  {
    GIVEN("An initialized Logger")
    {
      const int N = 1024*1024;
      char buffer[N] = { 0 };
      gsl::span<char> log{buffer, N};
      Logger logger{log};

      WHEN("Adding shitloads of entries")
      {
        const std::string entry{"This is a string which will fill 42 bytes"};
        const auto sz = entry.size() + 1;
        const auto max_entries = logger.size() / sz;
        const unsigned rotations = 3;

        for(int i; i < max_entries * rotations; ++i)
          logger.log(entry);

        THEN("There isnt more than maximum possible entries and they are all correct")
        {
          auto entries = logger.entries();

          EXPECT( entries.size() <= max_entries );

          bool all_correct = true;
          for(auto& ent : entries) {
            if(ent != entry) {
              all_correct = false;
              break;
            }
          }

          EXPECT( all_correct );
        }
      }
    }
  }

};

int main(int argc, char * argv[])
{
  printf("Running Logger tests\n");

  int res = lest::run(test_logger, argc, argv);

  printf("Tests completed.\n");

  return res;
}
