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

#include "../lest/include/lest/lest.hpp"
#include "../../route/path_to_regexp.cpp"

using namespace std;
using namespace route;

// ------------ TESTING PATHTOREGEXP CREATION WITH OPTIONS --------------

const lest::test test_path_to_regexp_options[] =
{
  SCENARIO("Creating PathToRegexp-objects with options")
  {
    // Create with option strict

    GIVEN("An empty vector of Tokens (keys) and option strict set to true")
    {
      vector<Token> keys;
      map<string, bool> options{ {"strict", true} };

      WHEN("Creating PathToRegexp-object with path ''")
      {

      }
    }/*,

    GIVEN("An empty vector of Tokens (keys) and option strict set to false")
    {
      vector<Token> keys;
      map<string, bool> options{ {"strict", false} };

      WHEN("Creating PathToRegexp-object with path ''")
      {

      }
    },

    // Create with option sensitive

    GIVEN("An empty vector of Tokens (keys) and option sensitive set to true")
    {
      vector<Token> keys;
      map<string, bool> options{ {"sensitive", true} };

      WHEN("Creating PathToRegexp-object with path ''")
      {

      }
    },

    GIVEN("An empty vector of Tokens (keys) and option sensitive set to false")
    {
      vector<Token> keys;
      map<string, bool> options{ {"sensitive", false} };

      WHEN("Creating PathToRegexp-object with path ''")
      {

      }
    },

    // Create with option end

    GIVEN("An empty vector of Tokens (keys) and option end set to true")
    {
      vector<Token> keys;
      map<string, bool> options{ {"end", true} };

      WHEN("Creating PathToRegexp-object with path ''")
      {

      }
    },

    GIVEN("An empty vector of Tokens (keys) and option end set to false")
    {
      vector<Token> keys;
      map<string, bool> options{ {"end", false} };

      WHEN("Creating PathToRegexp-object with path ''")
      {

      }
    },

    // Create with options strict and sensitive

    GIVEN("An empty vector of Tokens (keys) and options strict and sensitive set to true")
    {
      vector<Token> keys;
      map<string, bool> options{ {"strict", true}, {"sensitive", true} };

      WHEN("")
      {

      }
    },

    GIVEN("An empty vector of Tokens (keys) and options strict and sensitive set to false")
    {
      vector<Token> keys;
      map<string, bool> options{ {"strict", false}, {"sensitive", false} };

      WHEN("")
      {

      }
    },

    GIVEN("An empty vector of Tokens (keys) and option strict set to true and option sensitive set to false")
    {
      vector<Token> keys;
      map<string, bool> options{ {"strict", true}, {"sensitive", false} };

      WHEN("")
      {

      }
    },

    GIVEN("An empty vector of Tokens (keys) and option strict set to false and option sensitive set to true")
    {
      vector<Token> keys;
      map<string, bool> options{ {"strict", false}, {"sensitive", true} };

      WHEN("")
      {

      }
    },

    // Create with options strict and end

    GIVEN("An empty vector of Tokens (keys) and options strict and end set to true")
    {
      vector<Token> keys;
      map<string, bool> options{ {"strict", true}, {"end", true} };

      WHEN("")
      {

      }
    },

    GIVEN("An empty vector of Tokens (keys) and options strict and end set to false")
    {
      vector<Token> keys;
      map<string, bool> options{ {"strict", false}, {"end", false} };

      WHEN("")
      {

      }
    },

    GIVEN("An empty vector of Tokens (keys) and option strict set to true and option end set to false")
    {
      vector<Token> keys;
      map<string, bool> options{ {"strict", true}, {"end", false} };

      WHEN("")
      {

      }
    },

    GIVEN("An empty vector of Tokens (keys) and option strict set to false and option end set to true")
    {
      vector<Token> keys;
      map<string, bool> options{ {"strict", false}, {"end", true} };

      WHEN("")
      {

      }
    },

    // Create with options sensitive and end

    GIVEN("An empty vector of Tokens (keys) and options sensitive and end set to true")
    {
      vector<Token> keys;
      map<string, bool> options{ {"sensitive", true}, {"end", true} };

      WHEN("")
      {

      }
    },

    GIVEN("An empty vector of Tokens (keys) and options sensitive and end set to false")
    {
      vector<Token> keys;
      map<string, bool> options{ {"sensitive", false}, {"end", false} };

      WHEN("")
      {

      }
    },

    GIVEN("An empty vector of Tokens (keys) and option sensitive set to true and option end set to false")
    {
      vector<Token> keys;
      map<string, bool> options{ {"sensitive", true}, {"end", false} };

      WHEN("")
      {

      }
    },

    GIVEN("An empty vector of Tokens (keys) and option sensitive set to false and option end set to true")
    {
      vector<Token> keys;
      map<string, bool> options{ {"sensitive", false}, {"end", true} };

      WHEN("")
      {

      }
    },

    // Create with options strict, sensitive and end

    GIVEN("An empty vector of Tokens (keys) and options strict, sensitive and end set to true")
    {
      vector<Token> keys;
      map<string, bool> options{ {"strict", true}, {"sensitive", true}, {"end", true} };

      WHEN("")
      {

      }
    },

    GIVEN("An empty vector of Tokens (keys) and options strict, sensitive and end set to false")
    {
      vector<Token> keys;
      map<string, bool> options{ {"strict", false}, {"sensitive", false}, {"end", false} };

      WHEN("")
      {

      }
    },

    GIVEN("An empty vector of Tokens (keys) and option strict set to true, sensitive set to true and end set to false")
    {
      vector<Token> keys;
      map<string, bool> options{ {"strict", true}, {"sensitive", true}, {"end", false} };

      WHEN("")
      {

      }
    },

    GIVEN("An empty vector of Tokens (keys) and option strict set to true, sensitive set to false and end set to false")
    {
      vector<Token> keys;
      map<string, bool> options{ {"strict", true}, {"sensitive", false}, {"end", false} };

      WHEN("")
      {

      }
    },

    GIVEN("An empty vector of Tokens (keys) and option strict set to true, sensitive set to false and end set to true")
    {
      vector<Token> keys;
      map<string, bool> options{ {"strict", true}, {"sensitive", false}, {"end", true} };

      WHEN("")
      {

      }
    },

    GIVEN("An empty vector of Tokens (keys) and option strict set to false, sensitive set to false and end set to true")
    {
      vector<Token> keys;
      map<string, bool> options{ {"strict", false}, {"sensitive", false}, {"end", true} };

      WHEN("")
      {

      }
    },

    GIVEN("An empty vector of Tokens (keys) and option strict set to false, sensitive set to true and end set to true")
    {
      vector<Token> keys;
      map<string, bool> options{ {"strict", false}, {"sensitive", true}, {"end", true} };

      WHEN("")
      {

      }
    },

    GIVEN("An empty vector of Tokens (keys) and option strict set to false, sensitive set to true and end set to false")
    {
      vector<Token> keys;
      map<string, bool> options{ {"strict", false}, {"sensitive", true}, {"end", false} };

      WHEN("")
      {

      }
    }*/
  } // < SCENARIO (with options)
};

int main(int argc, char * argv[])
{
  printf("Running tests of PathToRegexp-creation with options...\n");

  int res = lest::run(test_path_to_regexp_options, argc, argv);

  printf("PathToRegexp creation-tests (with options) completed.\n");

  return res;
}
