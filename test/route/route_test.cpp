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

// --------------------- TESTING PARSE -------------------------

const lest::test test_path_to_regexp_parse[] =
{
  // --------------- Testing PathToRegexp parse-method ------------------

  // Testing named parameter

  CASE("String with one named parameter (/:test)")
  {
    // Empty object to use parse-method:
    vector<Token> keys;
    PathToRegexp p{"", keys};

    vector<Token> tokens = p.parse("/:test");

    EXPECT_NOT(tokens.empty());
    EXPECT(tokens.size() == 1u);

    Token t = tokens[0];

    EXPECT(t.name == "test");
    EXPECT(t.prefix == "/");
    EXPECT(t.delimiter == "/");
    EXPECT_NOT(t.optional);
    EXPECT_NOT(t.repeat);
    EXPECT_NOT(t.partial);
    EXPECT_NOT(t.asterisk);
    EXPECT(t.pattern == "[^/]+?");  // or "[^\/]+?"
    EXPECT_NOT(t.is_string);
  },

  CASE("String with two named parameters (/:test/:date)")
  {
    // Empty object to use parse-method:
    vector<Token> keys;
    PathToRegexp p{"", keys};

    vector<Token> tokens = p.parse("/:test/:date");

    EXPECT_NOT(tokens.empty());
    EXPECT(tokens.size() == 2u);

    Token t = tokens[0];
    Token t2 = tokens[1];

    EXPECT(t.name == "test");
    EXPECT(t.prefix == "/");
    EXPECT(t.delimiter == "/");
    EXPECT_NOT(t.optional);
    EXPECT_NOT(t.repeat);
    EXPECT_NOT(t.partial);
    EXPECT_NOT(t.asterisk);
    EXPECT(t.pattern == "[^/]+?");  // or "[^\/]+?"
    EXPECT_NOT(t.is_string);

    EXPECT(t2.name == "date");
    EXPECT(t2.prefix == "/");
    EXPECT(t2.delimiter == "/");
    EXPECT_NOT(t2.optional);
    EXPECT_NOT(t2.repeat);
    EXPECT_NOT(t2.partial);
    EXPECT_NOT(t2.asterisk);
    EXPECT(t2.pattern == "[^/]+?");  // or "[^\/]+?"
    EXPECT_NOT(t2.is_string);
  },

  CASE("String with three elements, containing two named parameters (/users/:test/:date)")
  {
    // Empty object to use parse-method:
    vector<Token> keys;
    PathToRegexp p{"", keys};

    vector<Token> tokens = p.parse("/users/:test/:date");

    EXPECT_NOT(tokens.empty());
    EXPECT(tokens.size() == 3u);

    Token t = tokens[0];
    Token t2 = tokens[1];
    Token t3 = tokens[2];

    EXPECT(t.name == "/users");
    EXPECT(t.prefix == "");
    EXPECT(t.delimiter == "");
    EXPECT_NOT(t.optional);
    EXPECT_NOT(t.repeat);
    EXPECT_NOT(t.partial);
    EXPECT_NOT(t.asterisk);
    EXPECT(t.pattern == "");  // or "[^\/]+?"
    EXPECT(t.is_string);

    EXPECT(t2.name == "test");
    EXPECT(t2.prefix == "/");
    EXPECT(t2.delimiter == "/");
    EXPECT_NOT(t2.optional);
    EXPECT_NOT(t2.repeat);
    EXPECT_NOT(t2.partial);
    EXPECT_NOT(t2.asterisk);
    EXPECT(t2.pattern == "[^/]+?");  // or "[^\/]+?"
    EXPECT_NOT(t2.is_string);

    EXPECT(t3.name == "date");
    EXPECT(t3.prefix == "/");
    EXPECT(t3.delimiter == "/");
    EXPECT_NOT(t3.optional);
    EXPECT_NOT(t3.repeat);
    EXPECT_NOT(t3.partial);
    EXPECT_NOT(t3.asterisk);
    EXPECT(t3.pattern == "[^/]+?");  // or "[^\/]+?"
    EXPECT_NOT(t3.is_string);
  },

  // Testing no parameters

  CASE("String with no parameters (/test)")
  {
    // Empty object to use parse-method:
    vector<Token> keys;
    PathToRegexp p{"", keys};

    vector<Token> tokens = p.parse("/test");

    EXPECT_NOT(tokens.empty());
    EXPECT(tokens.size() == 1u);

    Token t = tokens[0];

    EXPECT(t.name == "/test");
    EXPECT(t.prefix == "");
    EXPECT(t.delimiter == "");
    EXPECT_NOT(t.optional);
    EXPECT_NOT(t.repeat);
    EXPECT_NOT(t.partial);
    EXPECT_NOT(t.asterisk);
    EXPECT(t.pattern == "");
    EXPECT(t.is_string);
  },

  CASE("String with no parameters (/test/users)")
  {
    // Empty object to use parse-method:
    vector<Token> keys;
    PathToRegexp p{"", keys};

    vector<Token> tokens = p.parse("/test/users");

    EXPECT_NOT(tokens.empty());
    EXPECT(tokens.size() == 1u);

    Token t = tokens[0];

    EXPECT(t.name == "/test/users");
    EXPECT(t.prefix == "");
    EXPECT(t.delimiter == "");
    EXPECT_NOT(t.optional);
    EXPECT_NOT(t.repeat);
    EXPECT_NOT(t.partial);
    EXPECT_NOT(t.asterisk);
    EXPECT(t.pattern == "");
    EXPECT(t.is_string);
  },

  // Testing optional parameters ( ? )

  CASE("String with one optional parameter (/:test?)")
  {
    // Empty object to use parse-method:
    vector<Token> keys;
    PathToRegexp p{"", keys};

    vector<Token> tokens = p.parse("/:test?");

    EXPECT_NOT(tokens.empty());
    EXPECT(tokens.size() == 1u);

    Token t = tokens[0];

    EXPECT(t.name == "test");
    EXPECT(t.prefix == "/");
    EXPECT(t.delimiter == "/");
    EXPECT(t.optional);
    EXPECT_NOT(t.repeat);
    EXPECT_NOT(t.partial);
    EXPECT_NOT(t.asterisk);
    EXPECT(t.pattern == "[^/]+?");  // or "[^\/]+?"
    EXPECT_NOT(t.is_string);
  },

  CASE("String with two optional parameters (/:test?/:date?)")
  {
    // Empty object to use parse-method:
    vector<Token> keys;
    PathToRegexp p{"", keys};

    vector<Token> tokens = p.parse("/:test?/:date?");

    EXPECT_NOT(tokens.empty());
    EXPECT(tokens.size() == 2u);

    Token t = tokens[0];
    Token t2 = tokens[1];

    EXPECT(t.name == "test");
    EXPECT(t.prefix == "/");
    EXPECT(t.delimiter == "/");
    EXPECT(t.optional);
    EXPECT_NOT(t.repeat);
    EXPECT_NOT(t.partial);
    EXPECT_NOT(t.asterisk);
    EXPECT(t.pattern == "[^/]+?");  // or "[^\/]+?"
    EXPECT_NOT(t.is_string);

    EXPECT(t2.name == "date");
    EXPECT(t2.prefix == "/");
    EXPECT(t2.delimiter == "/");
    EXPECT(t2.optional);
    EXPECT_NOT(t2.repeat);
    EXPECT_NOT(t2.partial);
    EXPECT_NOT(t2.asterisk);
    EXPECT(t2.pattern == "[^/]+?");  // or "[^\/]+?"
    EXPECT_NOT(t2.is_string);
  },

  CASE("String with three elements, containing two optional parameters (/:test?/users/:date?)")
  {
    // Empty object to use parse-method:
    vector<Token> keys;
    PathToRegexp p{"", keys};

    vector<Token> tokens = p.parse("/:test?/users/:date?");

    EXPECT_NOT(tokens.empty());
    EXPECT(tokens.size() == 3u);

    Token t = tokens[0];
    Token t2 = tokens[1];
    Token t3 = tokens[2];

    EXPECT(t.name == "test");
    EXPECT(t.prefix == "/");
    EXPECT(t.delimiter == "/");
    EXPECT(t.optional);
    EXPECT_NOT(t.repeat);
    EXPECT_NOT(t.partial);
    EXPECT_NOT(t.asterisk);
    EXPECT(t.pattern == "[^/]+?");  // or "[^\/]+?"
    EXPECT_NOT(t.is_string);

    EXPECT(t2.name == "/users");
    EXPECT(t2.prefix == "");
    EXPECT(t2.delimiter == "");
    EXPECT_NOT(t2.optional);
    EXPECT_NOT(t2.repeat);
    EXPECT_NOT(t2.partial);
    EXPECT_NOT(t2.asterisk);
    EXPECT(t2.pattern == "");  // or "[^\/]+?"
    EXPECT(t2.is_string);

    EXPECT(t3.name == "date");
    EXPECT(t3.prefix == "/");
    EXPECT(t3.delimiter == "/");
    EXPECT(t3.optional);
    EXPECT_NOT(t3.repeat);
    EXPECT_NOT(t3.partial);
    EXPECT_NOT(t3.asterisk);
    EXPECT(t3.pattern == "[^/]+?");  // or "[^\/]+?"
    EXPECT_NOT(t3.is_string);
  },

  CASE("String with two named parameters, where one is optional (/:test/:date?)")
  {
    // Empty object to use parse-method:
    vector<Token> keys;
    PathToRegexp p{"", keys};

    vector<Token> tokens = p.parse("/:test/:date?");

    EXPECT_NOT(tokens.empty());
    EXPECT(tokens.size() == 2u);

    Token t = tokens[0];
    Token t2 = tokens[1];

    EXPECT(t.name == "test");
    EXPECT(t.prefix == "/");
    EXPECT(t.delimiter == "/");
    EXPECT_NOT(t.optional);
    EXPECT_NOT(t.repeat);
    EXPECT_NOT(t.partial);
    EXPECT_NOT(t.asterisk);
    EXPECT(t.pattern == "[^/]+?");  // or "[^\/]+?"
    EXPECT_NOT(t.is_string);

    EXPECT(t2.name == "date");
    EXPECT(t2.prefix == "/");
    EXPECT(t2.delimiter == "/");
    EXPECT(t2.optional);
    EXPECT_NOT(t2.repeat);
    EXPECT_NOT(t2.partial);
    EXPECT_NOT(t2.asterisk);
    EXPECT(t2.pattern == "[^/]+?");  // or "[^\/]+?"
    EXPECT_NOT(t2.is_string);
  },

  // Testing parameters with asterisk (zero or more)

  CASE("String with one named parameter with asterisk (zero or more) (/:test*)")
  {
    // Empty object to use parse-method:
    vector<Token> keys;
    PathToRegexp p{"", keys};

    vector<Token> tokens = p.parse("/:test*");

    EXPECT_NOT(tokens.empty());
    EXPECT(tokens.size() == 1u);

    Token t = tokens[0];

    EXPECT(t.name == "test");
    EXPECT(t.prefix == "/");
    EXPECT(t.delimiter == "/");
    EXPECT(t.optional);
    EXPECT(t.repeat);
    EXPECT_NOT(t.partial);
    EXPECT_NOT(t.asterisk);
    EXPECT(t.pattern == "[^/]+?");  // or "[^\/]+?"
    EXPECT_NOT(t.is_string);
  },

  CASE("String with two parameters, where one is a named parameter with asterisk (zero or more) (/:date/:test*)")
  {
    // Empty object to use parse-method:
    vector<Token> keys;
    PathToRegexp p{"", keys};

    vector<Token> tokens = p.parse("/:date/:test*");

    EXPECT_NOT(tokens.empty());
    EXPECT(tokens.size() == 2u);

    Token t = tokens[0];
    Token t2 = tokens[1];

    EXPECT(t.name == "date");
    EXPECT(t.prefix == "/");
    EXPECT(t.delimiter == "/");
    EXPECT_NOT(t.optional);
    EXPECT_NOT(t.repeat);
    EXPECT_NOT(t.partial);
    EXPECT_NOT(t.asterisk);
    EXPECT(t.pattern == "[^/]+?");  // or "[^\/]+?"
    EXPECT_NOT(t.is_string);

    EXPECT(t2.name == "test");
    EXPECT(t2.prefix == "/");
    EXPECT(t2.delimiter == "/");
    EXPECT(t2.optional);
    EXPECT(t2.repeat);
    EXPECT_NOT(t2.partial);
    EXPECT_NOT(t2.asterisk);
    EXPECT(t2.pattern == "[^/]+?");  // or "[^\/]+?"
    EXPECT_NOT(t2.is_string);
  },

  // Testing parameters with plus (one or more)

  CASE("String with one named parameter with plus (one or more) (/:test+)")
  {
    // Empty object to use parse-method:
    vector<Token> keys;
    PathToRegexp p{"", keys};

    vector<Token> tokens = p.parse("/:test+");

    EXPECT_NOT(tokens.empty());
    EXPECT(tokens.size() == 1u);

    Token t = tokens[0];

    EXPECT(t.name == "test");
    EXPECT(t.prefix == "/");
    EXPECT(t.delimiter == "/");
    EXPECT_NOT(t.optional);
    EXPECT(t.repeat);
    EXPECT_NOT(t.partial);
    EXPECT_NOT(t.asterisk);
    EXPECT(t.pattern == "[^/]+?");  // or "[^\/]+?"
    EXPECT_NOT(t.is_string);
  },

  CASE("String with two parameters, where one is a named parameter with plus (one or more) (/:id/:test+")
  {
    // Empty object to use parse-method:
    vector<Token> keys;
    PathToRegexp p{"", keys};

    vector<Token> tokens = p.parse("/:id/:test+");

    EXPECT_NOT(tokens.empty());
    EXPECT(tokens.size() == 2u);

    Token t = tokens[0];
    Token t2 = tokens[1];

    EXPECT(t.name == "id");
    EXPECT(t.prefix == "/");
    EXPECT(t.delimiter == "/");
    EXPECT_NOT(t.optional);
    EXPECT_NOT(t.repeat);
    EXPECT_NOT(t.partial);
    EXPECT_NOT(t.asterisk);
    EXPECT(t.pattern == "[^/]+?");  // or "[^\/]+?"
    EXPECT_NOT(t.is_string);

    EXPECT(t2.name == "test");
    EXPECT(t2.prefix == "/");
    EXPECT(t2.delimiter == "/");
    EXPECT_NOT(t2.optional);
    EXPECT(t2.repeat);
    EXPECT_NOT(t2.partial);
    EXPECT_NOT(t2.asterisk);
    EXPECT(t2.pattern == "[^/]+?"); // or "[^\/]+?"
    EXPECT_NOT(t2.is_string);
  },

  CASE("String with two parameters, where one is a named parameter with plus (one or more) that only takes lower case letters, and one is a named parameter that only takes integers (/:test([a-z])+/:id(\\d+))")
  {
    // Empty object to use parse-method:
    vector<Token> keys;
    PathToRegexp p{"", keys};

    vector<Token> tokens = p.parse("/:test([a-z])+/:id(\\d+)");

    EXPECT_NOT(tokens.empty());
    EXPECT(tokens.size() == 2u);

    Token t = tokens[0];
    Token t2 = tokens[1];

    EXPECT(t.name == "test");
    EXPECT(t.prefix == "/");
    EXPECT(t.delimiter == "/");
    EXPECT_NOT(t.optional);
    EXPECT(t.repeat);
    EXPECT_NOT(t.partial);
    EXPECT_NOT(t.asterisk);
    EXPECT(t.pattern == "[a-z]");
    EXPECT_NOT(t.is_string);

    EXPECT(t2.name == "id");
    EXPECT(t2.prefix == "/");
    EXPECT(t2.delimiter == "/");
    EXPECT_NOT(t2.optional);
    EXPECT_NOT(t2.repeat);
    EXPECT_NOT(t2.partial);
    EXPECT_NOT(t2.asterisk);
    EXPECT(t2.pattern == "\\d+");  // or "\d+"
    EXPECT_NOT(t2.is_string);
  },

  // Testing custom match parameters

  CASE("String with one custom match parameter - only integers (one or more) (/:test(\\d+))")
  {
    // Empty object to use parse-method:
    vector<Token> keys;
    PathToRegexp p{"", keys};

    vector<Token> tokens = p.parse("/:test(\\d+)");

    EXPECT_NOT(tokens.empty());
    EXPECT(tokens.size() == 1u);

    Token t = tokens[0];

    EXPECT(t.name == "test");
    EXPECT(t.prefix == "/");
    EXPECT(t.delimiter == "/");
    EXPECT_NOT(t.optional);
    EXPECT_NOT(t.repeat);
    EXPECT_NOT(t.partial);
    EXPECT_NOT(t.asterisk);
    EXPECT(t.pattern == "\\d+");  // or "\d+"
    EXPECT_NOT(t.is_string);
  },

  CASE("String with one custom match parameter - only a-z (one or more) (/:test([a-z]+))")
  {
    // Empty object to use parse-method:
    vector<Token> keys;
    PathToRegexp p{"", keys};

    vector<Token> tokens = p.parse("/:test([a-z]+)");

    EXPECT_NOT(tokens.empty());
    EXPECT(tokens.size() == 1u);

    Token t = tokens[0];

    EXPECT(t.name == "test");
    EXPECT(t.prefix == "/");
    EXPECT(t.delimiter == "/");
    EXPECT_NOT(t.optional);
    EXPECT_NOT(t.repeat);
    EXPECT_NOT(t.partial);
    EXPECT_NOT(t.asterisk);
    EXPECT(t.pattern == "[a-z]+");
    EXPECT_NOT(t.is_string);
  },

  CASE("String with two parameters, where one is a custom match parameter - only integers (one or more) (/:test/:id(\\d+))")
  {
    // Empty object to use parse-method:
    vector<Token> keys;
    PathToRegexp p{"", keys};

    vector<Token> tokens = p.parse("/:test/:id(\\d+)");

    EXPECT_NOT(tokens.empty());
    EXPECT(tokens.size() == 2u);

    Token t = tokens[0];
    Token t2 = tokens[1];

    EXPECT(t.name == "test");
    EXPECT(t.prefix == "/");
    EXPECT(t.delimiter == "/");
    EXPECT_NOT(t.optional);
    EXPECT_NOT(t.repeat);
    EXPECT_NOT(t.partial);
    EXPECT_NOT(t.asterisk);
    EXPECT(t.pattern == "[^/]+?");  // or "[^\/]+?"
    EXPECT_NOT(t.is_string);

    EXPECT(t2.name == "id");
    EXPECT(t2.prefix == "/");
    EXPECT(t2.delimiter == "/");
    EXPECT_NOT(t2.optional);
    EXPECT_NOT(t2.repeat);
    EXPECT_NOT(t2.partial);
    EXPECT_NOT(t2.asterisk);
    EXPECT(t2.pattern == "\\d+");  // or "\d+"
    EXPECT_NOT(t2.is_string);
  },

  // Testing unnamed parameters

  CASE("String with one unnamed parameter (/(.*)")
  {
    // Empty object to use parse-method:
    vector<Token> keys;
    PathToRegexp p{"", keys};

    vector<Token> tokens = p.parse("/(.*)");

    EXPECT_NOT(tokens.empty());
    EXPECT(tokens.size() == 1u);

    Token t = tokens[0];

    EXPECT(t.name == "0");
    EXPECT(t.prefix == "/");
    EXPECT(t.delimiter == "/");
    EXPECT_NOT(t.optional);
    EXPECT_NOT(t.repeat);
    EXPECT_NOT(t.partial);
    EXPECT_NOT(t.asterisk);
    EXPECT(t.pattern == ".*");
    EXPECT_NOT(t.is_string);
  },

  CASE("String with one unnamed parameter that only takes integers (one or more) (/(\\d+))")
  {
    // Empty object to use parse-method:
    vector<Token> keys;
    PathToRegexp p{"", keys};

    vector<Token> tokens = p.parse("/(\\d+)");

    EXPECT_NOT(tokens.empty());
    EXPECT(tokens.size() == 1u);

    Token t = tokens[0];

    EXPECT(t.name == "0");
    EXPECT(t.prefix == "/");
    EXPECT(t.delimiter == "/");
    EXPECT_NOT(t.optional);
    EXPECT_NOT(t.repeat);
    EXPECT_NOT(t.partial);
    EXPECT_NOT(t.asterisk);
    EXPECT(t.pattern == "\\d+");  // or \d+
    EXPECT_NOT(t.is_string);
  },

  CASE("String with two parameters, where one is an unnamed parameter (/:test/(.*))")
  {
    // Empty object to use parse-method:
    vector<Token> keys;
    PathToRegexp p{"", keys};

    vector<Token> tokens = p.parse("/:test/(.*)");

    EXPECT_NOT(tokens.empty());
    EXPECT(tokens.size() == 2u);

    Token t = tokens[0];
    Token t2 = tokens[1];

    EXPECT(t.name == "test");
    EXPECT(t.prefix == "/");
    EXPECT(t.delimiter == "/");
    EXPECT_NOT(t.optional);
    EXPECT_NOT(t.repeat);
    EXPECT_NOT(t.partial);
    EXPECT_NOT(t.asterisk);
    EXPECT(t.pattern == "[^/]+?");  // or "[^\/]+?"
    EXPECT_NOT(t.is_string);

    EXPECT(t2.name == "0");
    EXPECT(t2.prefix == "/");
    EXPECT(t2.delimiter == "/");
    EXPECT_NOT(t2.optional);
    EXPECT_NOT(t2.repeat);
    EXPECT_NOT(t2.partial);
    EXPECT_NOT(t2.asterisk);
    EXPECT(t2.pattern == ".*");
    EXPECT_NOT(t2.is_string);
  },

  CASE("String with two elements, where one is an unnamed parameter (/users/(.*))")
  {
    // Empty object to use parse-method:
    vector<Token> keys;
    PathToRegexp p{"", keys};

    vector<Token> tokens = p.parse("/users/(.*)");

    EXPECT_NOT(tokens.empty());
    EXPECT(tokens.size() == 2u);

    Token t = tokens[0];
    Token t2 = tokens[1];

    EXPECT(t.name == "/users");
    EXPECT(t.prefix == "");
    EXPECT(t.delimiter == "");
    EXPECT_NOT(t.optional);
    EXPECT_NOT(t.repeat);
    EXPECT_NOT(t.partial);
    EXPECT_NOT(t.asterisk);
    EXPECT(t.pattern == "");
    EXPECT(t.is_string);

    EXPECT(t2.name == "0");
    EXPECT(t2.prefix == "/");
    EXPECT(t2.delimiter == "/");
    EXPECT_NOT(t2.optional);
    EXPECT_NOT(t2.repeat);
    EXPECT_NOT(t2.partial);
    EXPECT_NOT(t2.asterisk);
    EXPECT(t2.pattern == ".*");
    EXPECT_NOT(t2.is_string);
  },

  // Testing asterisk parameter

  CASE("String with one unnamed parameter (/*)")
  {
    // Empty object to use parse-method:
    vector<Token> keys;
    PathToRegexp p{"", keys};

    vector<Token> tokens = p.parse("/*");

    EXPECT_NOT(tokens.empty());
    EXPECT(tokens.size() == 1u);

    Token t = tokens[0];

    EXPECT(t.name == "0");
    EXPECT(t.prefix == "/");
    EXPECT(t.delimiter == "/");
    EXPECT_NOT(t.optional);
    EXPECT_NOT(t.repeat);
    EXPECT_NOT(t.partial);
    EXPECT(t.asterisk);
    EXPECT(t.pattern == ".*");
    EXPECT_NOT(t.is_string);
  },

  CASE("String with two parameters, where one is an asterisk (zero or more) (/test/:id/*)")
  {
    // Empty object to use parse-method:
    vector<Token> keys;
    PathToRegexp p{"", keys};

    vector<Token> tokens = p.parse("/test/:id/*");

    EXPECT_NOT(tokens.empty());
    EXPECT(tokens.size() == 3u);

    Token t = tokens[0];
    Token t2 = tokens[1];
    Token t3 = tokens[2];

    EXPECT(t.name == "/test");
    EXPECT(t.prefix == "");
    EXPECT(t.delimiter == "");
    EXPECT_NOT(t.optional);
    EXPECT_NOT(t.repeat);
    EXPECT_NOT(t.partial);
    EXPECT_NOT(t.asterisk);
    EXPECT(t.pattern == "");
    EXPECT(t.is_string);

    EXPECT(t2.name == "id");
    EXPECT(t2.prefix == "/");
    EXPECT(t2.delimiter == "/");
    EXPECT_NOT(t2.optional);
    EXPECT_NOT(t2.repeat);
    EXPECT_NOT(t2.partial);
    EXPECT_NOT(t2.asterisk);
    EXPECT(t2.pattern == "[^/]+?"); // or "[^\/]+?"
    EXPECT_NOT(t2.is_string);

    EXPECT(t3.name == "0");
    EXPECT(t3.prefix == "/");
    EXPECT(t3.delimiter == "/");
    EXPECT_NOT(t3.optional);
    EXPECT_NOT(t3.repeat);
    EXPECT_NOT(t3.partial);
    EXPECT(t3.asterisk);
    EXPECT(t3.pattern == ".*");
    EXPECT_NOT(t3.is_string);
  }
};

// ----------------- TESTING TOKENS_TO_REGEXP -------------------------

const lest::test test_path_to_regexp_tokens[] =
{

  // const std::regex tokens_to_regexp(const std::vector<Token>& tokens, const std::map<std::string, bool>& options) const;

  /*CASE("")
  {

  },

  CASE("")
  {

  }*/
};

// --------------------- TESTING CREATION -------------------------

const lest::test test_path_to_regexp_creation[] =
{
  // --------------- Testing PathToRegexp-creation ------------------

  SCENARIO("Creating PathToRegexp-objects with no options")
  {
    GIVEN("An empty vector of Tokens (keys) and no options")
    {
      vector<Token> keys;

      WHEN("Creating PathToRegexp-object with empty path")
      {
        PathToRegexp p{"", keys};

        // Testing keys:

        EXPECT(keys.empty());
        EXPECT(keys.size() == 0u);

        // Testing regex:

        WHEN("Getting the regex")
        {
          std::regex r = p.get_regex();

          THEN("No paths should match")
          {
            EXPECT(std::regex_match("", r));

            EXPECT_NOT(std::regex_match("/route", r));
            EXPECT_NOT(std::regex_match("/route/:id", r));

      // How to test if expected regex is correct? Is this the only option (run regex_match)?

          }
        }
      }

      AND_WHEN("Creating PathToRegexp-object with path '/:test'")
      {
        PathToRegexp p{"/:test", keys};

        // Testing keys:

        EXPECT_NOT(keys.empty());
        EXPECT(keys.size() == 1u);

        Token t = keys[0];

        // Tested when testing parse-method really, except keys don't contain Tokens where is_string is true
        EXPECT(t.name == "test");
        EXPECT(t.prefix == "/");
        EXPECT(t.delimiter == "/");
        EXPECT_NOT(t.optional);
        EXPECT_NOT(t.repeat);
        EXPECT_NOT(t.partial);
        EXPECT_NOT(t.asterisk);
        EXPECT(t.pattern == "[^/]+?");  // or "[^\/]+?"
        EXPECT_NOT(t.is_string);

        // Testing regex:

        WHEN("Getting the regex")
        {
          std::regex r = p.get_regex();

          THEN("Only paths with one parameter should match. This can contain any character")
          {
            EXPECT_NOT(std::regex_match("/", r));
            EXPECT_NOT(std::regex_match("/route/123", r));
            EXPECT_NOT(std::regex_match("/route/something/somethingelse12", r));
            EXPECT_NOT(std::regex_match("route", r));

            EXPECT(std::regex_match("/route", r));
            EXPECT(std::regex_match("/123", r));
            EXPECT(std::regex_match("/route!)#̈́'", r));

        // How to test if expected regex is correct? Is this the only option (run regex_match)?

          }
        }
      }

      WHEN("Creating PathToRegexp-object with path '/:test/:date'")
      {
        PathToRegexp p{"/:test/:date", keys};

        // Testing keys:

        EXPECT_NOT(keys.empty());
        EXPECT(keys.size() == 2u);

        Token t = keys[0];
        Token t2 = keys[1];

        // Tested when testing parse-method really, except keys don't contain Tokens where is_string is true
        EXPECT(t.name == "test");
        EXPECT(t.prefix == "/");
        EXPECT(t.delimiter == "/");
        EXPECT_NOT(t.optional);
        EXPECT_NOT(t.repeat);
        EXPECT_NOT(t.partial);
        EXPECT_NOT(t.asterisk);
        EXPECT(t.pattern == "[^/]+?");  // or "[^\/]+?"
        EXPECT_NOT(t.is_string);

        EXPECT(t2.name == "date");
        EXPECT(t2.prefix == "/");
        EXPECT(t2.delimiter == "/");
        EXPECT_NOT(t2.optional);
        EXPECT_NOT(t2.repeat);
        EXPECT_NOT(t2.partial);
        EXPECT_NOT(t2.asterisk);
        EXPECT(t2.pattern == "[^/]+?");  // or "[^\/]+?"
        EXPECT_NOT(t2.is_string);

        // Testing regex:

        WHEN("Getting the regex")
        {
          std::regex r = p.get_regex();

          THEN("Only paths with two parameters should match. This can contain any character")
          {
            EXPECT_NOT(std::regex_match("/", r));
            EXPECT_NOT(std::regex_match("/route", r));
            EXPECT_NOT(std::regex_match("/route/something/somethingelse12", r));

            EXPECT(std::regex_match("/route/123", r));
            EXPECT(std::regex_match("/123/route", r));
            EXPECT(std::regex_match("/route!)#/route321'", r));
          }
        }
      }

      WHEN("Creating PathToRegexp-object with path '/users/:test/:date'")
      {
        PathToRegexp p{"/users/:test/:date", keys};

        // Testing keys:

        EXPECT_NOT(keys.empty());
        EXPECT(keys.size() == 2u);

        Token t = keys[0];
        Token t2 = keys[1];

        // Tested when testing parse-method really, except keys don't contain Tokens where is_string is true
        EXPECT(t.name == "test");
        EXPECT(t.prefix == "/");
        EXPECT(t.delimiter == "/");
        EXPECT_NOT(t.optional);
        EXPECT_NOT(t.repeat);
        EXPECT_NOT(t.partial);
        EXPECT_NOT(t.asterisk);
        EXPECT(t.pattern == "[^/]+?");  // or "[^\/]+?"
        EXPECT_NOT(t.is_string);

        EXPECT(t2.name == "date");
        EXPECT(t2.prefix == "/");
        EXPECT(t2.delimiter == "/");
        EXPECT_NOT(t2.optional);
        EXPECT_NOT(t2.repeat);
        EXPECT_NOT(t2.partial);
        EXPECT_NOT(t2.asterisk);
        EXPECT(t2.pattern == "[^/]+?");  // or "[^\/]+?"
        EXPECT_NOT(t2.is_string);

        // Testing regex:

        WHEN("Getting the regex")
        {
          std::regex r = p.get_regex();

          THEN("Only paths with three elements should match, starting with /users")
          {
            EXPECT_NOT(std::regex_match("/", r));
            EXPECT_NOT(std::regex_match("/route", r));
            EXPECT_NOT(std::regex_match("/users", r));
            EXPECT_NOT(std::regex_match("/users/123", r));
            EXPECT_NOT(std::regex_match("/users/something/123/else", r));
            EXPECT_NOT(std::regex_match("/route/something/somethingelse12", r));

            EXPECT(std::regex_match("/users/123/something", r));
            EXPECT(std::regex_match("/users/some12p-?/523", r));
            EXPECT(std::regex_match("/users/route!)#/route321'", r));
          }
        }
      }

      WHEN("Creating PathToRegexp-object with path '/test'")
      {
        PathToRegexp p{"/test", keys};

        // Testing keys:

        EXPECT(keys.empty());
        EXPECT(keys.size() == 0u);

        // Testing regex:

        WHEN("Getting the regex")
        {
          std::regex r = p.get_regex();

          THEN("Only the path '/test' should match")
          {
            EXPECT_NOT(std::regex_match("/", r));
            EXPECT_NOT(std::regex_match("/tes", r));
            EXPECT_NOT(std::regex_match("/tests", r));
            EXPECT_NOT(std::regex_match("/test/123", r));
            EXPECT_NOT(std::regex_match("/test/something/somethingelse12", r));

            EXPECT(std::regex_match("/test", r));
          }
        }
      }

      WHEN("Creating PathToRegexp-object with path '/test/users'")
      {
        PathToRegexp p{"/test/users", keys};

        // Testing keys:

        EXPECT(keys.empty());
        EXPECT(keys.size() == 0u);

        // Testing regex:

        WHEN("Getting the regex")
        {
          std::regex r = p.get_regex();

          THEN("Only the path '/test/users' should match")
          {
            EXPECT_NOT(std::regex_match("/", r));
            EXPECT_NOT(std::regex_match("/test", r));
            EXPECT_NOT(std::regex_match("/test/user", r));
            EXPECT_NOT(std::regex_match("/tes/users", r));
            EXPECT_NOT(std::regex_match("/test/users/123", r));
            EXPECT_NOT(std::regex_match("/test/something/somethingelse12", r));

            EXPECT(std::regex_match("/test/users", r));
          }
        }
      }

      WHEN("Creating PathToRegexp-object with path '/:test?'")
      {
        PathToRegexp p{"/:test?", keys};

        // Testing keys:

        EXPECT_NOT(keys.empty());
        EXPECT(keys.size() == 1u);

        Token t = keys[0];

        // Tested when testing parse-method really, except keys don't contain Tokens where is_string is true
        EXPECT(t.name == "test");
        EXPECT(t.prefix == "/");
        EXPECT(t.delimiter == "/");
        EXPECT(t.optional);
        EXPECT_NOT(t.repeat);
        EXPECT_NOT(t.partial);
        EXPECT_NOT(t.asterisk);
        EXPECT(t.pattern == "[^/]+?");  // or "[^\/]+?"
        EXPECT_NOT(t.is_string);

        // Testing regex:

        WHEN("Getting the regex")
        {
          std::regex r = p.get_regex();

          THEN("Only paths with one or no parameters should match")
          {
            EXPECT_NOT(std::regex_match("/users/123", r));
            EXPECT_NOT(std::regex_match("/users/something/123/else", r));
            EXPECT_NOT(std::regex_match("/route/something/somethingelse12", r));

            EXPECT(std::regex_match("", r));
            EXPECT(std::regex_match("/", r));
            EXPECT(std::regex_match("/route", r));
            EXPECT(std::regex_match("/users4)", r));
            EXPECT(std::regex_match("/123", r));
          }
        }
      }

      WHEN("Creating PathToRegexp-object with path '/:test?/:date?'")
      {
        PathToRegexp p{"/:test?/:date?", keys};

        // Testing keys:

        EXPECT_NOT(keys.empty());
        EXPECT(keys.size() == 2u);

        Token t = keys[0];
        Token t2 = keys[1];

        // Tested when testing parse-method really, except keys don't contain Tokens where is_string is true
        EXPECT(t.name == "test");
        EXPECT(t.prefix == "/");
        EXPECT(t.delimiter == "/");
        EXPECT(t.optional);
        EXPECT_NOT(t.repeat);
        EXPECT_NOT(t.partial);
        EXPECT_NOT(t.asterisk);
        EXPECT(t.pattern == "[^/]+?");  // or "[^\/]+?"
        EXPECT_NOT(t.is_string);

        EXPECT(t2.name == "date");
        EXPECT(t2.prefix == "/");
        EXPECT(t2.delimiter == "/");
        EXPECT(t2.optional);
        EXPECT_NOT(t2.repeat);
        EXPECT_NOT(t2.partial);
        EXPECT_NOT(t2.asterisk);
        EXPECT(t2.pattern == "[^/]+?");  // or "[^\/]+?"
        EXPECT_NOT(t2.is_string);

        // Testing regex:

        WHEN("Getting the regex")
        {
          std::regex r = p.get_regex();

          THEN("Only paths with zero, one or two parameters should match")
          {
            EXPECT_NOT(std::regex_match("/matilda/123/2016-08-20", r));
            EXPECT_NOT(std::regex_match("/users/matilda/2016-08-20/something", r));
            EXPECT_NOT(std::regex_match("/route/something/somethingelse12", r));

            EXPECT(std::regex_match("", r));
            EXPECT(std::regex_match("/", r));
            EXPECT(std::regex_match("/matilda", r));
            EXPECT(std::regex_match("/2016-08-20", r));
            EXPECT(std::regex_match("/2016-08-20/matilda", r));
            EXPECT(std::regex_match("/matilda/2016-08-20", r));
          }
        }
      }

      WHEN("Creating PathToRegexp-object with path '/:test?/users/:date?'")
      {
        PathToRegexp p{"/:test?/users/:date?", keys};

        // Testing keys:

        EXPECT_NOT(keys.empty());
        EXPECT(keys.size() == 2u);

        Token t = keys[0];
        Token t2 = keys[1];

        // Tested when testing parse-method really, except keys don't contain Tokens where is_string is true
        EXPECT(t.name == "test");
        EXPECT(t.prefix == "/");
        EXPECT(t.delimiter == "/");
        EXPECT(t.optional);
        EXPECT_NOT(t.repeat);
        EXPECT_NOT(t.partial);
        EXPECT_NOT(t.asterisk);
        EXPECT(t.pattern == "[^/]+?");  // or "[^\/]+?"
        EXPECT_NOT(t.is_string);

        EXPECT(t2.name == "date");
        EXPECT(t2.prefix == "/");
        EXPECT(t2.delimiter == "/");
        EXPECT(t2.optional);
        EXPECT_NOT(t2.repeat);
        EXPECT_NOT(t2.partial);
        EXPECT_NOT(t2.asterisk);
        EXPECT(t2.pattern == "[^/]+?");  // or "[^\/]+?"
        EXPECT_NOT(t2.is_string);

        // Testing regex:

        WHEN("Getting the regex")
        {
          std::regex r = p.get_regex();

          THEN("Only paths with zero parameters (but containing /users), one parameter (but containing /users), or two parameters (but containing /users) should match")
          {
            EXPECT_NOT(std::regex_match("/", r));
            EXPECT_NOT(std::regex_match("/users/something/matilda123", r));
            EXPECT_NOT(std::regex_match("/matilda/123/2016-08-20", r));
            EXPECT_NOT(std::regex_match("/users/matilda/2016-08-20/something", r));
            EXPECT_NOT(std::regex_match("/route/users/somethingelse12/123", r));
            EXPECT_NOT(std::regex_match("/123/matilda/users", r));

            EXPECT(std::regex_match("/users", r));
            EXPECT(std::regex_match("/matilda/users", r));
            EXPECT(std::regex_match("/matilda/users/2016-08-20", r));
            EXPECT(std::regex_match("/users/2016-08-20", r));
          }
        }
      }

      WHEN("Creating PathToRegexp-object with path '/:test/:date?'")
      {
        PathToRegexp p{"/:test/:date?", keys};


      }

      WHEN("Creating PathToRegexp-object with path '/:test*'")
      {
        PathToRegexp p{"/:test*", keys};


      }

      WHEN("Creating PathToRegexp-object with path '/:date/:test*'")
      {
        PathToRegexp p{"/:date/:test*", keys};


      }

      WHEN("Creating PathToRegexp-object with path '/:test+'")
      {
        PathToRegexp p{"/:test+", keys};


      }

      WHEN("Creating PathToRegexp-object with path '/:id/:test+'")
      {
        PathToRegexp p{"/:id/:test+", keys};


      }

      WHEN("Creating PathToRegexp-object with path '/:test([a-z])+/:id(\\d+)'")
      {
        PathToRegexp p{"/:test([a-z])+/:id(\\d+)", keys};


      }

      WHEN("Creating PathToRegexp-object with path '/:test(\\d+)'")
      {
        PathToRegexp p{"/:test(\\d+)", keys};


      }

      WHEN("Creating PathToRegexp-object with path '/:test([a-z]+)'")
      {
        PathToRegexp p{"/:test([a-z]+)", keys};


      }

      WHEN("Creating PathToRegexp-object with path '/:test/:id(\\d+)'")
      {
        PathToRegexp p{"/:test/:id(\\d+)", keys};


      }

      WHEN("Creating PathToRegexp-object with path '/(.*)'")
      {
        PathToRegexp p{"/(.*)", keys};


      }

      WHEN("Creating PathToRegexp-object with path '/(\\d+)'")
      {
        PathToRegexp p{"/(\\d+)", keys};


      }

      WHEN("Creating PathToRegexp-object with path '/:test/(.*)'")
      {
        PathToRegexp p{"/:test/(.*)", keys};


      }

      WHEN("Creating PathToRegexp-object with path '/users/(.*)'")
      {
        PathToRegexp p{"/users/(.*)", keys};


      }

      WHEN("Creating PathToRegexp-object with path '/*'")
      {
        PathToRegexp p{"/*", keys};


      }

      WHEN("Creating PathToRegexp-object with path '/test/:id/*'")
      {
        PathToRegexp p{"/test/:id/*", keys};


      }

      WHEN("Creating PathToRegexp-object with path '/:id(\\d+)/:date/*'")
      {

      }

      WHEN("Creating PathToRegexp-object with path '/:id([a-z]+)/:date/(.*)'")
      {

      }
    }
  },  // < SCENARIO 1 (no options)
/*
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
    },

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
    }
  } // < SCENARIO 2 (with options)
*/

/*
 Main:

  std::vector<route::Token> keys;

  //std::regex re = pathToRegexp{"/:test(\\d+)?", keys};
  //route::PathToRegexp r{"/:test(\\d+)?", keys};

  //route::PathToRegexp r{"/:test/:id", keys};

  //route::PathToRegexp r{"/user/:test(\\d+)/noe", keys};

  //route::PathToRegexp r{"/:test([a-z]+)/:dato/(users|players)", keys};

  //route::PathToRegexp r{"/:test*)", keys};

  //route::PathToRegexp r{"/*", keys};

  //route::PathToRegexp r{"/(apple-)?icon-:res(\\d+).png", keys};

  // Test escaped correct:
  // route::PathToRegexp r{"/route/:foo/\\a", keys};

  //route::PathToRegexp r{"/route/:foo/\\a", keys, std::map<std::string, bool>{ {"sensitive", false} }};
  //route::PathToRegexp r{"/:test(\\d+)", keys, std::map<std::string, bool>{ {"sensitive", true}, {"strict", true} }};

  // TEST 1
  // route::PathToRegexp path_to_regexp{"/:foo/:bar", keys};

  // TEST 2
  // route::PathToRegexp path_to_regexp{"/USERS/:foo/:bar", keys, std::map<std::string, bool>{ {"sensitive", true} }};

  // TEST 3
  // route::PathToRegexp path_to_regexp{"/:foo(\\d+)", keys, std::map<std::string, bool>{ {"sensitive", false} }};

  // TEST 4
  // route::PathToRegexp path_to_regexp{"/(apple-)?icon-:res(\\d+).png", keys};

  // TEST 5
  // route::PathToRegexp path_to_regexp{"/:foo/:bar?", keys};

  // TEST 6
  // route::PathToRegexp path_to_regexp{"/:foo*", keys};

  // TEST 7
  // route::PathToRegexp path_to_regexp{"/:foo+", keys};

  // TEST 8
  // route::PathToRegexp path_to_regexp{"/:foo/(.*)", keys};

  // TEST 9
  route::PathToRegexp path_to_regexp{"/foo/*", keys};
*/
  /*
   JS:
    var re = pathToRegexp('/:foo/:bar', keys)
    // keys = [{ name: 'foo', prefix: '/', ... }, { name: 'bar', prefix: '/', ... }]

    re.exec('/test/route')
    //=> ['/test/route', 'test', 'route']
   */

  /* Checking keys-variable (vector of Tokens):
  for(size_t i = 0; i < keys.size(); i++) {
    route::Token t = keys[i];
*/
    /* Token:
      std::string name; // can be a string or an int (index)
      std::string prefix;
      std::string delimiter;
      bool optional;
      bool repeat;
      bool partial;
      bool asterisk;
      std::string pattern;
     */
/*
    printf("Token/Key nr. %lu name: %s\n", i, (t.name).c_str());
    printf("Token/Key nr. %lu prefix: %s\n", i, (t.prefix).c_str());
    printf("Token/Key nr. %lu delimiter: %s\n", i, (t.delimiter).c_str());
    printf("Token/Key nr. %lu optional: %s\n", i, t.optional ? "true" : "false");
    printf("Token/Key nr. %lu repeat: %s\n", i, t.repeat ? "true" : "false");
    printf("Token/Key nr. %lu pattern: %s\n", i, (t.pattern).c_str());

    printf("Token/Key nr. %lu partial: %s\n", i, t.partial ? "true" : "false");
    printf("Token/Key nr. %lu asterisk: %s\n", i, t.asterisk ? "true" : "false");
  }

  // Checking resulting regex:
  std::regex result = path_to_regexp.get_regex();
  */
  // TEST 1
  /*if(std::regex_match("/test/route", result))
    printf("/test/route MATCH\n");
  else
    printf("/test/route NO MATCH\n");

  if(std::regex_match("/test", result))
    printf("/test MATCH\n");
  else
    printf("/test NO MATCH\n");*/

  // TEST 2
  /*if(std::regex_match("/USERS/something/somethingelse", result))
    printf("/USERS/something/somethingelse MATCH\n");
  else
    printf("/USERS/something/somethingelse NO MATCH\n");

  if(std::regex_match("/users/something/somethingelse", result))
    printf("/users/something/somethingelse MATCH\n");
  else
    printf("/users/something/somethingelse NO MATCH\n");*/

  // TEST 3
  /*if(std::regex_match("/test", result))
    printf("/test MATCH\n");
  else
    printf("/test NO MATCH\n");

  if(std::regex_match("/123", result))
    printf("/123 MATCH\n");
  else
    printf("/123 NO MATCH\n");*/

  // TEST 4
  /*if(std::regex_match("/icon-76.png", result))
    printf("/icon-76.png MATCH\n");
  else
    printf("/icon-76.png NO MATCH\n");

  if(std::regex_match("/icon.png", result))
    printf("/icon.png MATCH\n");
  else
    printf("/icon.png NO MATCH\n");*/

  // TEST 5
  /*if(std::regex_match("/test", result))
    printf("/test MATCH\n");
  else
    printf("/test NO MATCH\n");

  if(std::regex_match("/test/route", result))
    printf("/test/route MATCH\n");
  else
    printf("/test/route NO MATCH\n");

  if(std::regex_match("/test/route/something", result))
    printf("/test/route/something MATCH\n");
  else
    printf("/test/route/something NO MATCH\n");*/

  // TEST 6 and 7
  /*if(std::regex_match("/", result))
    printf("/ MATCH\n");
  else
    printf("/ NO MATCH\n");

  if(std::regex_match("/bar/baz", result))
    printf("/bar/baz MATCH\n");
  else
    printf("/bar/baz NO MATCH\n");*/

  // TEST 8
  /*if(std::regex_match("/test/route", result))
    printf("/test/route MATCH\n");
  else
    printf("/test/route NO MATCH\n");

  if(std::regex_match("/", result))
    printf("/ MATCH\n");
  else
    printf("/ NO MATCH\n");*/

  /* TEST 9
  if(std::regex_match("/foo/route", result))
    printf("/foo/route MATCH\n");
  else
    printf("/foo/route NO MATCH\n");

  if(std::regex_match("/foo/bar/baz", result))
    printf("/foo/bar/baz MATCH\n");
  else
    printf("/foo/bar/baz NO MATCH\n");

  if(std::regex_match("/bar/baz", result))
    printf("/bar/baz MATCH\n");
  else
    printf("/bar/baz NO MATCH\n");

  // Also test returned array (iterate through): TEST 8 f.ex.: "/test/route" => ['/test/route', 'test', 'route']

 */
};

int main(int argc, char * argv[])
{
  printf("Running tests of PathToRegexp parse-method...\n");

  int res = lest::run(test_path_to_regexp_parse, argc, argv);

  printf("PathToRegexp parse-tests completed.\n");

  /*printf("Running tests of PathToRegexp tokens_to_regexp-method...\n");

  int res = lest::run(test_path_to_regexp_tokens, argc, argv);

  printf("PathToRegexp tokens-tests completed.\n");*/

  printf("Running tests of PathToRegexp-creation...\n");

  res = lest::run(test_path_to_regexp_creation, argc, argv);

  printf("PathToRegexp creation-tests completed.\n");

  return res;
}
