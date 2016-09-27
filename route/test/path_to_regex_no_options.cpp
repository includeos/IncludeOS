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

// ------------ TESTING PATH_TO_REGEX WITH NO OPTIONS --------------

const lest::test test_path_to_regex_no_options[] =
{
  SCENARIO("Creating path_to_regex with no options")
  {
    GIVEN("An empty vector of Tokens (keys) and no options")
    {
      Tokens keys;

      WHEN("Calling path_to_regex with empty path")
      {
        std::regex r = path_to_regex("", keys);

        // Testing keys:

        EXPECT(keys.empty());
        EXPECT(keys.size() == 0u);

        // Testing regex:

        THEN("No paths should match")
        {
          EXPECT(std::regex_match("", r));

          EXPECT_NOT(std::regex_match("/route", r));
          EXPECT_NOT(std::regex_match("/route/123", r));
        }
      }

      WHEN("Calling path_to_regex with path '/:test'")
      {
        std::regex r = path_to_regex("/:test", keys);

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

        THEN("Only paths with one parameter should match. This can contain any character")
        {
          EXPECT_NOT(std::regex_match("/", r));
          EXPECT_NOT(std::regex_match("/route/123", r));
          EXPECT_NOT(std::regex_match("/route/something/somethingelse12", r));
          EXPECT_NOT(std::regex_match("route", r));

          EXPECT(std::regex_match("/route", r));
          EXPECT(std::regex_match("/123", r));
          EXPECT(std::regex_match("/route!)#̈́'", r));
        }
      }

      WHEN("Calling path_to_regex with path '/:test/:date'")
      {
        std::regex r = path_to_regex("/:test/:date", keys);

        // Testing keys:

        EXPECT_NOT(keys.empty());
        EXPECT(keys.size() == 2u);

        Token t1 = keys[0];
        Token t2 = keys[1];

        // Tested when testing parse-method really, except keys don't contain Tokens where is_string is true
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

        // Testing regex:

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

      WHEN("Calling path_to_regex with path '/users/:test/:date'")
      {
        std::regex r = path_to_regex("/users/:test/:date", keys);

        // Testing keys:

        EXPECT_NOT(keys.empty());
        EXPECT(keys.size() == 2u);

        Token t1 = keys[0];
        Token t2 = keys[1];

        // Tested when testing parse-method really, except keys don't contain Tokens where is_string is true
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

        // Testing regex:

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

      WHEN("Calling path_to_regex with path '/test'")
      {
        std::regex r = path_to_regex("/test", keys);

        // Testing keys:

        EXPECT(keys.empty());
        EXPECT(keys.size() == 0u);

        // Testing regex:

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

      WHEN("Calling path_to_regex with path '/test/users'")
      {
        std::regex r = path_to_regex("/test/users", keys);

        // Testing keys:

        EXPECT(keys.empty());
        EXPECT(keys.size() == 0u);

        // Testing regex:

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

      WHEN("Calling path_to_regex with path '/:test?'")
      {
        std::regex r = path_to_regex("/:test?", keys);

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

      WHEN("Calling path_to_regex with path '/:test?/:date?'")
      {
        std::regex r = path_to_regex("/:test?/:date?", keys);

        // Testing keys:

        EXPECT_NOT(keys.empty());
        EXPECT(keys.size() == 2u);

        Token t1 = keys[0];
        Token t2 = keys[1];

        // Tested when testing parse-method really, except keys don't contain Tokens where is_string is true
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

        // Testing regex:

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

      WHEN("Calling path_to_regex with path '/:test?/users/:date?'")
      {
        std::regex r = path_to_regex("/:test?/users/:date?", keys);

        // Testing keys:

        EXPECT_NOT(keys.empty());
        EXPECT(keys.size() == 2u);

        Token t1 = keys[0];
        Token t2 = keys[1];

        // Tested when testing parse-method really, except keys don't contain Tokens where is_string is true
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

        // Testing regex:

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

      WHEN("Calling path_to_regex with path '/:test/:date?'")
      {
        std::regex r = path_to_regex("/:test/:date?", keys);

        // Testing keys:

        EXPECT_NOT(keys.empty());
        EXPECT(keys.size() == 2u);

        Token t1 = keys[0];
        Token t2 = keys[1];

        // Tested when testing parse-method really, except keys don't contain Tokens where is_string is true
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

        // Testing regex:

        THEN("Only paths with one or two parameters should match")
        {
          EXPECT_NOT(std::regex_match("/", r));
          EXPECT_NOT(std::regex_match("/users/something/matilda123", r));
          EXPECT_NOT(std::regex_match("/matilda/123/2016-08-20", r));
          EXPECT_NOT(std::regex_match("/test/matilda/2016-08-20/something", r));

          EXPECT(std::regex_match("/users", r));
          EXPECT(std::regex_match("/matilda/users", r));
          EXPECT(std::regex_match("/matilda/2016-08-20", r));
        }
      }

      WHEN("Calling path_to_regex with path '/:test*'")
      {
        std::regex r = path_to_regex("/:test*", keys);

        // Testing keys:

        EXPECT_NOT(keys.empty());
        EXPECT(keys.size() == 1u);

        Token t = keys[0];

        // Tested when testing parse-method really, except keys don't contain Tokens where is_string is true
        EXPECT(t.name == "test");
        EXPECT(t.prefix == "/");
        EXPECT(t.delimiter == "/");
        EXPECT(t.optional);
        EXPECT(t.repeat);
        EXPECT_NOT(t.partial);
        EXPECT_NOT(t.asterisk);
        EXPECT(t.pattern == "[^/]+?");  // or "[^\/]+?"
        EXPECT_NOT(t.is_string);

        // Testing regex:

        THEN("Paths with zero or more parameters should match")
        {
          EXPECT(std::regex_match("/", r));
          EXPECT(std::regex_match("/users", r));
          EXPECT(std::regex_match("/matilda/users", r));
          EXPECT(std::regex_match("/matilda/2016-08-20", r));
          EXPECT(std::regex_match("/users/something/matilda123", r));
          EXPECT(std::regex_match("/test/something/matilda123/2016-08-15/somethingelse/testing", r));
        }
      }

      WHEN("Calling path_to_regex with path '/:date/:test*'")
      {
        std::regex r = path_to_regex("/:date/:test*", keys);

        // Testing keys:

        EXPECT_NOT(keys.empty());
        EXPECT(keys.size() == 2u);

        Token t1 = keys[0];
        Token t2 = keys[1];

        // Tested when testing parse-method really, except keys don't contain Tokens where is_string is true
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

        // Testing regex:

        THEN("Paths with one or more parameters should match")
        {
          EXPECT_NOT(std::regex_match("/", r));

          EXPECT(std::regex_match("/users", r));
          EXPECT(std::regex_match("/matilda/users", r));
          EXPECT(std::regex_match("/matilda/2016-08-20", r));
          EXPECT(std::regex_match("/users/something/matilda123", r));
          EXPECT(std::regex_match("/test/something/matilda123/2016-08-15/somethingelse/testing", r));
        }
      }

      WHEN("Calling path_to_regex with path '/:test+'")
      {
        std::regex r = path_to_regex("/:test+", keys);

        // Testing keys:

        EXPECT_NOT(keys.empty());
        EXPECT(keys.size() == 1u);

        Token t = keys[0];

        // Tested when testing parse-method really, except keys don't contain Tokens where is_string is true
        EXPECT(t.name == "test");
        EXPECT(t.prefix == "/");
        EXPECT(t.delimiter == "/");
        EXPECT_NOT(t.optional);
        EXPECT(t.repeat);
        EXPECT_NOT(t.partial);
        EXPECT_NOT(t.asterisk);
        EXPECT(t.pattern == "[^/]+?");  // or "[^\/]+?"
        EXPECT_NOT(t.is_string);

        // Testing regex:

        THEN("Paths with one or more parameters should match")
        {
          EXPECT_NOT(std::regex_match("", r));
          EXPECT_NOT(std::regex_match("/", r));

          EXPECT(std::regex_match("/users", r));
          EXPECT(std::regex_match("/matilda/users", r));
          EXPECT(std::regex_match("/matilda/2016-08-20", r));
          EXPECT(std::regex_match("/users/something/matilda123", r));
          EXPECT(std::regex_match("/test/something/matilda123/2016-08-15/somethingelse/testing", r));
        }
      }

      WHEN("Calling path_to_regex with path '/:id/:test+'")
      {
        std::regex r = path_to_regex("/:id/:test+", keys);

        // Testing keys:

        EXPECT_NOT(keys.empty());
        EXPECT(keys.size() == 2u);

        Token t1 = keys[0];
        Token t2 = keys[1];

        // Tested when testing parse-method really, except keys don't contain Tokens where is_string is true
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
        EXPECT(t2.pattern == "[^/]+?");  // or "[^\/]+?"
        EXPECT_NOT(t2.is_string);

        // Testing regex:

        THEN("Paths with two or more parameters should match")
        {
          EXPECT_NOT(std::regex_match("/", r));
          EXPECT_NOT(std::regex_match("/users", r));

          EXPECT(std::regex_match("/matilda/users", r));
          EXPECT(std::regex_match("/matilda/2016-08-20", r));
          EXPECT(std::regex_match("/users/something/matilda123", r));
          EXPECT(std::regex_match("/test/something/matilda123/2016-08-15/somethingelse/testing", r));
        }
      }

      WHEN("Calling path_to_regex with path '/:test([a-z])+/:id(\\d+)'")
      {
        std::regex r = path_to_regex("/:test([a-z])+/:id(\\d+)", keys);

        // Testing keys:

        EXPECT_NOT(keys.empty());
        EXPECT(keys.size() == 2u);

        Token t1 = keys[0];
        Token t2 = keys[1];

        // Tested when testing parse-method really, except keys don't contain Tokens where is_string is true
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
        EXPECT(t2.pattern == "\\d+"); // or \d+
        EXPECT_NOT(t2.is_string);

        // Testing regex:

        THEN("Paths with two parameters (or more of the first parameter) where the first contains one lower case letter and the second contains one or more integers should match")
        {
          EXPECT_NOT(std::regex_match("/", r));
          EXPECT_NOT(std::regex_match("/m", r));
          EXPECT_NOT(std::regex_match("/molly/123", r));
          EXPECT_NOT(std::regex_match("/m/u", r));
          EXPECT_NOT(std::regex_match("/M/1234", r));
          EXPECT_NOT(std::regex_match("/1234/1234", r));
          EXPECT_NOT(std::regex_match("/4/1234", r));
          EXPECT_NOT(std::regex_match("/m/1234/1234", r));
          EXPECT_NOT(std::regex_match("/b/2016-08-20", r));
          EXPECT_NOT(std::regex_match("/z/", r));

          EXPECT(std::regex_match("/m/2", r));
          EXPECT(std::regex_match("/z/1234", r));
          EXPECT(std::regex_match("/t/s/1234", r));
          EXPECT(std::regex_match("/a/z/m/2016", r));
        }
      }

      WHEN("Calling path_to_regex with path '/:test(\\d+)'")
      {
        std::regex r = path_to_regex("/:test(\\d+)", keys);

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
        EXPECT(t.pattern == "\\d+");  // or "\d+"
        EXPECT_NOT(t.is_string);

        // Testing regex:

        THEN("Paths with one parameter that contains one or more integers should match")
        {
          EXPECT_NOT(std::regex_match("", r));
          EXPECT_NOT(std::regex_match("/", r));
          EXPECT_NOT(std::regex_match("/users", r));
          EXPECT_NOT(std::regex_match("/12/25", r));

          EXPECT(std::regex_match("/5", r));
          EXPECT(std::regex_match("/2016", r));
          EXPECT(std::regex_match("/1234563728192938", r));
        }
      }

      // TODO
      WHEN("Calling path_to_regex with path '/:test([a-z]+)'")
      {
        // Everything works as expected when is case sensitive
        // But when setting the icase constant (ignore case) it doesn't

        /* Basic example for testing icase:

        std::regex test_regex("[a-z]+");
        EXPECT_NOT(std::regex_match("A", test_regex));
        EXPECT_NOT(std::regex_match("B", test_regex));
        EXPECT(std::regex_match("a", test_regex));
        EXPECT(std::regex_match("b", test_regex));

        std::regex test_regex_2("[a-z]+", std::regex_constants::ECMAScript | std::regex_constants::icase);
        EXPECT(std::regex_match("A", test_regex_2));  // OK
        EXPECT(std::regex_match("B", test_regex_2));  // FAILS
        EXPECT(std::regex_match("C", test_regex_2));  // FAILS
        EXPECT(std::regex_match("a", test_regex_2));
        EXPECT(std::regex_match("b", test_regex_2));

        // Basic example for testing icase end */

        // Not case sensitive (default) - does not work as expected:
        std::regex r = path_to_regex("/:test([a-z]+)", keys);

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
        EXPECT(t.pattern == "[a-z]+");
        EXPECT_NOT(t.is_string);

        // Testing regex:

        THEN("Paths with one parameter containing one or more lower case letters should match")
        {
          EXPECT_NOT(std::regex_match("", r));
          EXPECT_NOT(std::regex_match("/", r));
          EXPECT_NOT(std::regex_match("/matilda/users", r));
          EXPECT_NOT(std::regex_match("/1", r));
          EXPECT_NOT(std::regex_match("/4321", r));

          // Default is not case sensitive
          EXPECT(std::regex_match("/A", r));        // OK WHEN NOT CASE SENSITIVE (icase set)
          EXPECT(std::regex_match("/B", r));        // FAILS WHEN NOT CASE SENSITIVE (icase set)
          EXPECT(std::regex_match("/Z", r));        // FAILS WHEN NOT CASE SENSITIVE (icase set)
          EXPECT(std::regex_match("/Matilda", r));  // FAILS WHEN NOT CASE SENSITIVE (icase set)

          EXPECT(std::regex_match("/a", r));
          EXPECT(std::regex_match("/users", r));
          EXPECT(std::regex_match("/somethingelseentirely", r));
        }
      }

      WHEN("Calling path_to_regex with path '/:test/' (endsWithSlash)")
      {
        std::regex r = path_to_regex("/:test/", keys);

        THEN("The trailing slash will be ignored in non-strict mode (strict is false)")
        {
          EXPECT(std::regex_match("/123", r));
        }
      }

      WHEN("Calling path_to_regex with path '/:test/:id(\\d+)'")
      {
        std::regex r = path_to_regex("/:test/:id(\\d+)", keys);

        // Testing keys:

        EXPECT_NOT(keys.empty());
        EXPECT(keys.size() == 2u);

        Token t1 = keys[0];
        Token t2 = keys[1];

        // Tested when testing parse-method really, except keys don't contain Tokens where is_string is true
        EXPECT(t1.name == "test");
        EXPECT(t1.prefix == "/");
        EXPECT(t1.delimiter == "/");
        EXPECT_NOT(t1.optional);
        EXPECT_NOT(t1.repeat);
        EXPECT_NOT(t1.partial);
        EXPECT_NOT(t1.asterisk);
        EXPECT(t1.pattern == "[^/]+?");  // or [^\/]+?
        EXPECT_NOT(t1.is_string);

        EXPECT(t2.name == "id");
        EXPECT(t2.prefix == "/");
        EXPECT(t2.delimiter == "/");
        EXPECT_NOT(t2.optional);
        EXPECT_NOT(t2.repeat);
        EXPECT_NOT(t2.partial);
        EXPECT_NOT(t2.asterisk);
        EXPECT(t2.pattern == "\\d+"); // or \d+
        EXPECT_NOT(t2.is_string);

        // Testing regex:

        THEN("Paths with two parameters where the second contains one or more integers should match")
        {
          EXPECT_NOT(std::regex_match("/", r));
          EXPECT_NOT(std::regex_match("/m", r));
          EXPECT_NOT(std::regex_match("/molly", r));
          EXPECT_NOT(std::regex_match("/molly/", r));
          EXPECT_NOT(std::regex_match("/molly1234/users", r));
          EXPECT_NOT(std::regex_match("/1234", r));
          EXPECT_NOT(std::regex_match("/m/1234/1234", r));
          EXPECT_NOT(std::regex_match("/users/molly/2016", r));

          EXPECT(std::regex_match("/m/0", r));
          EXPECT(std::regex_match("/molly/123", r));
          EXPECT(std::regex_match("/1234/1234", r));
          EXPECT(std::regex_match("/USERSusers1520!/1234", r));
          EXPECT(std::regex_match("/t/0192830", r));
          EXPECT(std::regex_match("/panel/2016", r));
        }
      }

      WHEN("Calling path_to_regex with path '/(.*)'")
      {
        std::regex r = path_to_regex("/(.*)", keys);

        // Testing keys:

        EXPECT_NOT(keys.empty());
        EXPECT(keys.size() == 1u);

        Token t = keys[0];

        // Tested when testing parse-method really, except keys don't contain Tokens where is_string is true
        EXPECT(t.name == "0");
        EXPECT(t.prefix == "/");
        EXPECT(t.delimiter == "/");
        EXPECT_NOT(t.optional);
        EXPECT_NOT(t.repeat);
        EXPECT_NOT(t.partial);
        EXPECT_NOT(t.asterisk);
        EXPECT(t.pattern == ".*");
        EXPECT_NOT(t.is_string);

        // Testing regex:

        THEN("Paths with one parameter containing zero or more characters should match")
        {
          EXPECT_NOT(std::regex_match("", r));

          EXPECT(std::regex_match("/matilda/users", r));  // Because of .*: Any character allowed, incl. /
          EXPECT(std::regex_match("/", r));
          EXPECT(std::regex_match("/Z", r));
          EXPECT(std::regex_match("/A", r));
          EXPECT(std::regex_match("/B132ab", r));
          EXPECT(std::regex_match("/1", r));
          EXPECT(std::regex_match("/4321", r));
        }
      }

      // TODO
      WHEN("Calling path_to_regex with path '/users/([a-z]+)/(.?)'")
      {
        std::regex r = path_to_regex("/users/([a-z]+)/(.?)", keys);

        // Testing keys:

        EXPECT_NOT(keys.empty());
        EXPECT(keys.size() == 2u);

        Token t1 = keys[0];
        Token t2 = keys[1];

        // Tested when testing parse-method really, except keys don't contain Tokens where is_string is true
        EXPECT(t1.name == "0");
        EXPECT(t1.prefix == "/");
        EXPECT(t1.delimiter == "/");
        EXPECT_NOT(t1.optional);
        EXPECT_NOT(t1.repeat);
        EXPECT_NOT(t1.partial);
        EXPECT_NOT(t1.asterisk);
        EXPECT(t1.pattern == "[a-z]+");
        EXPECT_NOT(t1.is_string);

        EXPECT(t2.name == "1");
        EXPECT(t2.prefix == "/");
        EXPECT(t2.delimiter == "/");
        EXPECT_NOT(t2.optional);
        EXPECT_NOT(t2.repeat);
        EXPECT_NOT(t2.partial);
        EXPECT_NOT(t2.asterisk);
        EXPECT(t2.pattern == ".?");
        EXPECT_NOT(t2.is_string);

        // Testing regex:

        THEN("Paths with three elements, where the first parameter contains one or more lower case letters and the second parameter contains zero or one character, should match")
        {
          EXPECT_NOT(std::regex_match("/", r));
          EXPECT_NOT(std::regex_match("/users", r));
          EXPECT_NOT(std::regex_match("/users/a/ab", r));
          EXPECT_NOT(std::regex_match("/user/users", r));
          EXPECT_NOT(std::regex_match("/users/1234", r));

          // TODO: /A is allowed (same error as above)
          // While /B is not allowed
          // Default (is the case here) is that the regex is not case sensitive (sensitive is false)
          EXPECT(std::regex_match("/users/A/a", r));
          EXPECT(std::regex_match("/users/B/a", r));

          EXPECT_NOT(std::regex_match("/m/abcd/2", r));
          EXPECT_NOT(std::regex_match("/users/molly/2016", r));
          EXPECT_NOT(std::regex_match("/users/abba1520!/t", r));

          EXPECT(std::regex_match("/users/a/0", r));
          EXPECT(std::regex_match("/users/ab/a", r));
          EXPECT(std::regex_match("/users/saab/", r));
          EXPECT(std::regex_match("/users/some/w", r));
          EXPECT(std::regex_match("/users/p/2", r));
        }
      }

      WHEN("Calling path_to_regex with path '/*'")
      {
        std::regex r = path_to_regex("/*", keys);

        // Testing keys:

        EXPECT_NOT(keys.empty());
        EXPECT(keys.size() == 1u);

        Token t = keys[0];

        // Tested when testing parse-method really, except keys don't contain Tokens where is_string is true
        EXPECT(t.name == "0");
        EXPECT(t.prefix == "/");
        EXPECT(t.delimiter == "/");
        EXPECT_NOT(t.optional);
        EXPECT_NOT(t.repeat);
        EXPECT_NOT(t.partial);
        EXPECT(t.asterisk);
        EXPECT(t.pattern == ".*");
        EXPECT_NOT(t.is_string);

        // Testing regex:

        THEN("Paths with one parameter containing zero or more characters should match")
        {
          EXPECT_NOT(std::regex_match("", r));

          EXPECT(std::regex_match("/matilda/users", r));  // Because of .*: Any character allowed, incl. /
          EXPECT(std::regex_match("/", r));
          EXPECT(std::regex_match("/Z", r));
          EXPECT(std::regex_match("/A", r));
          EXPECT(std::regex_match("/B132ab", r));
          EXPECT(std::regex_match("/1", r));
          EXPECT(std::regex_match("/4321", r));
        }
      }

      WHEN("Calling path_to_regex with path '/:id(\\d+)/:date/*'")
      {
        std::regex r = path_to_regex("/:id(\\d+)/:date/*", keys);

        // Testing keys:

        EXPECT_NOT(keys.empty());
        EXPECT(keys.size() == 3u);

        Token t1 = keys[0];
        Token t2 = keys[1];
        Token t3 = keys[2];

        // Tested when testing parse-method really, except keys don't contain Tokens where is_string is true
        EXPECT(t1.name == "id");
        EXPECT(t1.prefix == "/");
        EXPECT(t1.delimiter == "/");
        EXPECT_NOT(t1.optional);
        EXPECT_NOT(t1.repeat);
        EXPECT_NOT(t1.partial);
        EXPECT_NOT(t1.asterisk);
        EXPECT(t1.pattern == "\\d+");
        EXPECT_NOT(t1.is_string);

        EXPECT(t2.name == "date");
        EXPECT(t2.prefix == "/");
        EXPECT(t2.delimiter == "/");
        EXPECT_NOT(t2.optional);
        EXPECT_NOT(t2.repeat);
        EXPECT_NOT(t2.partial);
        EXPECT_NOT(t2.asterisk);
        EXPECT(t2.pattern == "[^/]+?");  // or [^\/]+?
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

        // Testing regex:

        THEN("Paths with three parameters, where the first parameter contains one or more integers and the third contains zero or more characters, should match")
        {
          EXPECT_NOT(std::regex_match("", r));
          EXPECT_NOT(std::regex_match("/b/optional-text/optional", r));
          EXPECT_NOT(std::regex_match("/1/matilda", r));
          EXPECT_NOT(std::regex_match("/172!optional-text/abcd2918230", r));

          EXPECT(std::regex_match("/0/optional-text/", r));
          EXPECT(std::regex_match("/0/optional-text/1", r));
          EXPECT(std::regex_match("/123/optional-text132/45", r));
          EXPECT(std::regex_match("/2918293/172!optional-text/abcd2918230", r));
          EXPECT(std::regex_match("/2/143/", r));
          EXPECT(std::regex_match("/2/143/something", r));
          EXPECT(std::regex_match("/2/143/something/somethingelse/and/so/on", r));  // Because of *: Any character allowed, incl. /
        }
      }
    }
  }  // < SCENARIO (no options)
};

int main(int argc, char * argv[])
{
  printf("Running tests of path_to_regex with no options...\n");

  int res = lest::run(test_path_to_regex_no_options, argc, argv);

  printf("Path_to_regex-tests (with no options) completed.\n");

  return res;
}
