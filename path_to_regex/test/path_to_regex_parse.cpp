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

#include <lest/lest.hpp>
#include "../path_to_regex.hpp"

using namespace std;
using namespace path2regex;

// ---------------- TESTING PATHTOREGEXP PARSE ---------------------

const lest::test test_path_to_regexp_parse[] =
{
  // Testing named parameter

  CASE("String with one named parameter (/:test)")
  {
    Tokens tokens = parse("/:test");

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
    Tokens tokens = parse("/:test/:date");

    EXPECT_NOT(tokens.empty());
    EXPECT(tokens.size() == 2u);

    Token t1 = tokens[0];
    Token t2 = tokens[1];

    EXPECT(t1.name == "test");
    EXPECT(t1.prefix == "/");
    EXPECT(t1.delimiter == "/");
    EXPECT_NOT(t1.optional);
    EXPECT_NOT(t1.repeat);
    EXPECT_NOT(t1.partial);
    EXPECT_NOT(t1.asterisk);
    EXPECT(t1.pattern == "[^/]+?");  // or "[^\/]+?"
    EXPECT_NOT(t1.is_string);

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
    Tokens tokens = parse("/users/:test/:date");

    EXPECT_NOT(tokens.empty());
    EXPECT(tokens.size() == 3u);

    Token t1 = tokens[0];
    Token t2 = tokens[1];
    Token t3 = tokens[2];

    EXPECT(t1.name == "/users");
    EXPECT(t1.prefix == "");
    EXPECT(t1.delimiter == "");
    EXPECT_NOT(t1.optional);
    EXPECT_NOT(t1.repeat);
    EXPECT_NOT(t1.partial);
    EXPECT_NOT(t1.asterisk);
    EXPECT(t1.pattern == "");  // or "[^\/]+?"
    EXPECT(t1.is_string);

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
    Tokens tokens = parse("/test");

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
    Tokens tokens = parse("/test/users");

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
    Tokens tokens = parse("/:test?");

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
    Tokens tokens = parse("/:test?/:date?");

    EXPECT_NOT(tokens.empty());
    EXPECT(tokens.size() == 2u);

    Token t1 = tokens[0];
    Token t2 = tokens[1];

    EXPECT(t1.name == "test");
    EXPECT(t1.prefix == "/");
    EXPECT(t1.delimiter == "/");
    EXPECT(t1.optional);
    EXPECT_NOT(t1.repeat);
    EXPECT_NOT(t1.partial);
    EXPECT_NOT(t1.asterisk);
    EXPECT(t1.pattern == "[^/]+?");  // or "[^\/]+?"
    EXPECT_NOT(t1.is_string);

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
    Tokens tokens = parse("/:test?/users/:date?");

    EXPECT_NOT(tokens.empty());
    EXPECT(tokens.size() == 3u);

    Token t1 = tokens[0];
    Token t2 = tokens[1];
    Token t3 = tokens[2];

    EXPECT(t1.name == "test");
    EXPECT(t1.prefix == "/");
    EXPECT(t1.delimiter == "/");
    EXPECT(t1.optional);
    EXPECT_NOT(t1.repeat);
    EXPECT_NOT(t1.partial);
    EXPECT_NOT(t1.asterisk);
    EXPECT(t1.pattern == "[^/]+?");  // or "[^\/]+?"
    EXPECT_NOT(t1.is_string);

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
    Tokens tokens = parse("/:test/:date?");

    EXPECT_NOT(tokens.empty());
    EXPECT(tokens.size() == 2u);

    Token t1 = tokens[0];
    Token t2 = tokens[1];

    EXPECT(t1.name == "test");
    EXPECT(t1.prefix == "/");
    EXPECT(t1.delimiter == "/");
    EXPECT_NOT(t1.optional);
    EXPECT_NOT(t1.repeat);
    EXPECT_NOT(t1.partial);
    EXPECT_NOT(t1.asterisk);
    EXPECT(t1.pattern == "[^/]+?");  // or "[^\/]+?"
    EXPECT_NOT(t1.is_string);

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
    Tokens tokens = parse("/:test*");

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
    Tokens tokens = parse("/:date/:test*");

    EXPECT_NOT(tokens.empty());
    EXPECT(tokens.size() == 2u);

    Token t1 = tokens[0];
    Token t2 = tokens[1];

    EXPECT(t1.name == "date");
    EXPECT(t1.prefix == "/");
    EXPECT(t1.delimiter == "/");
    EXPECT_NOT(t1.optional);
    EXPECT_NOT(t1.repeat);
    EXPECT_NOT(t1.partial);
    EXPECT_NOT(t1.asterisk);
    EXPECT(t1.pattern == "[^/]+?");  // or "[^\/]+?"
    EXPECT_NOT(t1.is_string);

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
    Tokens tokens = parse("/:test+");

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
    Tokens tokens = parse("/:id/:test+");

    EXPECT_NOT(tokens.empty());
    EXPECT(tokens.size() == 2u);

    Token t1 = tokens[0];
    Token t2 = tokens[1];

    EXPECT(t1.name == "id");
    EXPECT(t1.prefix == "/");
    EXPECT(t1.delimiter == "/");
    EXPECT_NOT(t1.optional);
    EXPECT_NOT(t1.repeat);
    EXPECT_NOT(t1.partial);
    EXPECT_NOT(t1.asterisk);
    EXPECT(t1.pattern == "[^/]+?");  // or "[^\/]+?"
    EXPECT_NOT(t1.is_string);

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
    Tokens tokens = parse("/:test([a-z])+/:id(\\d+)");

    EXPECT_NOT(tokens.empty());
    EXPECT(tokens.size() == 2u);

    Token t1 = tokens[0];
    Token t2 = tokens[1];

    EXPECT(t1.name == "test");
    EXPECT(t1.prefix == "/");
    EXPECT(t1.delimiter == "/");
    EXPECT_NOT(t1.optional);
    EXPECT(t1.repeat);
    EXPECT_NOT(t1.partial);
    EXPECT_NOT(t1.asterisk);
    EXPECT(t1.pattern == "[a-z]");
    EXPECT_NOT(t1.is_string);

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
    Tokens tokens = parse("/:test(\\d+)");

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
    Tokens tokens = parse("/:test([a-z]+)");

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
    Tokens tokens = parse("/:test/:id(\\d+)");

    EXPECT_NOT(tokens.empty());
    EXPECT(tokens.size() == 2u);

    Token t1 = tokens[0];
    Token t2 = tokens[1];

    EXPECT(t1.name == "test");
    EXPECT(t1.prefix == "/");
    EXPECT(t1.delimiter == "/");
    EXPECT_NOT(t1.optional);
    EXPECT_NOT(t1.repeat);
    EXPECT_NOT(t1.partial);
    EXPECT_NOT(t1.asterisk);
    EXPECT(t1.pattern == "[^/]+?");  // or "[^\/]+?"
    EXPECT_NOT(t1.is_string);

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
    Tokens tokens = parse("/(.*)");

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
    Tokens tokens = parse("/(\\d+)");

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
    Tokens tokens = parse("/:test/(.*)");

    EXPECT_NOT(tokens.empty());
    EXPECT(tokens.size() == 2u);

    Token t1 = tokens[0];
    Token t2 = tokens[1];

    EXPECT(t1.name == "test");
    EXPECT(t1.prefix == "/");
    EXPECT(t1.delimiter == "/");
    EXPECT_NOT(t1.optional);
    EXPECT_NOT(t1.repeat);
    EXPECT_NOT(t1.partial);
    EXPECT_NOT(t1.asterisk);
    EXPECT(t1.pattern == "[^/]+?");  // or "[^\/]+?"
    EXPECT_NOT(t1.is_string);

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
    Tokens tokens = parse("/users/(.*)");

    EXPECT_NOT(tokens.empty());
    EXPECT(tokens.size() == 2u);

    Token t1 = tokens[0];
    Token t2 = tokens[1];

    EXPECT(t1.name == "/users");
    EXPECT(t1.prefix == "");
    EXPECT(t1.delimiter == "");
    EXPECT_NOT(t1.optional);
    EXPECT_NOT(t1.repeat);
    EXPECT_NOT(t1.partial);
    EXPECT_NOT(t1.asterisk);
    EXPECT(t1.pattern == "");
    EXPECT(t1.is_string);

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
    Tokens tokens = parse("/*");

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
    Tokens tokens = parse("/test/:id/*");

    EXPECT_NOT(tokens.empty());
    EXPECT(tokens.size() == 3u);

    Token t1 = tokens[0];
    Token t2 = tokens[1];
    Token t3 = tokens[2];

    EXPECT(t1.name == "/test");
    EXPECT(t1.prefix == "");
    EXPECT(t1.delimiter == "");
    EXPECT_NOT(t1.optional);
    EXPECT_NOT(t1.repeat);
    EXPECT_NOT(t1.partial);
    EXPECT_NOT(t1.asterisk);
    EXPECT(t1.pattern == "");
    EXPECT(t1.is_string);

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

int main(int argc, char * argv[])
{
  printf("Running tests of PathToRegexp parse-method...\n");

  int res = lest::run(test_path_to_regexp_parse, argc, argv);

  printf("PathToRegexp parse-tests completed.\n");

  return res;
}
