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

#include "../cookie_jar.hpp"
#include <lest/lest.hpp>

using namespace std;
using namespace cookie;

const lest::test test_cookie_jar[] =
{
  SCENARIO("Cookies can be inserted into and erased from a CookieJar")
  {
    GIVEN("A CookieJar")
    {
      /* TODO: Not working when declared the CookieJar here for some reason:
      CookieJar jar{};
      EXPECT(jar.empty());
      EXPECT(jar.size() == 0u);*/

      WHEN("Inserting a Cookie into a CookieJar")
      {
        // TODO Moved here to make it work:
        CookieJar jar{};
        EXPECT(jar.empty());
        EXPECT(jar.size() == 0u);
        // Until here

        Cookie c{"name", "value"};
        EXPECT(jar.insert(c));

        THEN("The CookieJar contains the cookie")
        {
          EXPECT_NOT(jar.empty());
          EXPECT(jar.size() == 1u);
          EXPECT(jar.cookie_value("name") == "value");
          EXPECT(jar.exists("name"));
        }

        AND_WHEN("Erasing the inserted Cookie from the CookieJar")
        {
          EXPECT_NOT(jar.empty());
          jar.erase(c);

          THEN("The CookieJar does not contain the Cookie")
          {
            EXPECT(jar.empty());
            EXPECT(jar.size() == 0u);
            EXPECT(jar.cookie_value("name") == "");
            EXPECT_NOT(jar.exists("name"));
          }
        }
      }

      WHEN("Inserting three Cookies into a CookieJar by inserting name and value")
      {
        // TODO Moved here to make it work:
        CookieJar jar{};
        EXPECT(jar.empty());
        EXPECT(jar.size() == 0u);
        // Until here

        EXPECT(jar.insert("another_name", "another_value"));

        THEN("The CookieJar contains a Cookie with this name and value")
        {
          EXPECT_NOT(jar.empty());
          EXPECT(jar.size() == 1u);
          EXPECT(jar.cookie_value("another_name") == "another_value");
          EXPECT(jar.exists("another_name"));
        }

        EXPECT(jar.insert("a_cookie_name", "a_cookie_value"));

        THEN("The CookieJar contains both Cookies")
        {
          EXPECT_NOT(jar.empty());
          EXPECT(jar.size() == 2u);
          EXPECT(jar.cookie_value("another_name") == "another_value");
          EXPECT(jar.cookie_value("a_cookie_name") == "a_cookie_value");
          EXPECT(jar.exists("another_name"));
          EXPECT(jar.exists("a_cookie_name"));
        }

        EXPECT(jar.insert("a_third_cookie_name", "a_third_cookie_value"));

        THEN("The CookieJar contains three Cookies")
        {
          EXPECT_NOT(jar.empty());
          EXPECT(jar.size() == 3u);
          EXPECT(jar.cookie_value("another_name") == "another_value");
          EXPECT(jar.cookie_value("a_cookie_name") == "a_cookie_value");
          EXPECT(jar.cookie_value("a_third_cookie_name") == "a_third_cookie_value");
          EXPECT(jar.exists("another_name"));
          EXPECT(jar.exists("a_cookie_name"));
          EXPECT(jar.exists("a_third_cookie_name"));
        }

        AND_WHEN("Erasing one of the Cookies from the CookieJar by name")
        {
          EXPECT_NOT(jar.empty());
          jar.erase("a_cookie_name");

          THEN("The CookieJar contains only the other two Cookies")
          {
            EXPECT_NOT(jar.empty());
            EXPECT(jar.size() == 2u);
            EXPECT(jar.cookie_value("another_name") == "another_value");
            EXPECT(jar.cookie_value("a_third_cookie_name") == "a_third_cookie_value");
            EXPECT_NOT(jar.cookie_value("a_cookie_name") == "a_cookie_value");
            EXPECT(jar.exists("another_name"));
            EXPECT(jar.exists("a_third_cookie_name"));
            EXPECT_NOT(jar.exists("a_cookie_name"));
          }
        }

        AND_WHEN("Clearing the whole CookieJar")
        {
          EXPECT_NOT(jar.empty());
          EXPECT(jar.size() == 3u);
          jar.clear();

          THEN("The CookieJar is empty")
          {
            EXPECT(jar.empty());
            EXPECT(jar.size() == 0u);
            EXPECT(jar.cookie_value("another_name") == "");
            EXPECT_NOT(jar.exists("a_third_cookie_name"));
          }
        }

        AND_WHEN("Getting all the cookies that the CookieJar contains")
        {
          map<string, string> all_cookies = jar.get_cookies();

          THEN("The existing Cookies are returned in a map")
          {
            EXPECT_NOT(all_cookies.empty());
            EXPECT(all_cookies.size() == 3u);

            EXPECT(all_cookies.find("another_name") not_eq all_cookies.end());
            EXPECT(all_cookies.find("a_cookie_name") not_eq all_cookies.end());
            EXPECT(all_cookies.find("a_third_cookie_name") not_eq all_cookies.end());
            EXPECT(all_cookies.find("not_existing_cookie") == all_cookies.end());
          }
        }
      }
    }
  },

  SCENARIO("A CookieJar can return an iterator to its first and last element")
  {
    GIVEN("A CookieJar")
    {
      CookieJar jar{};

      WHEN("Inserting three Cookies into the CookieJar")
      {
        jar.insert("name", "value");
        jar.insert("another_name", "another_value");
        jar.insert("a_third_name", "a_third_value");

        EXPECT(jar.size() == 3u);

        THEN("Get an iterator to the first Cookie")
        {
          auto it = jar.begin();
          EXPECT( (it not_eq jar.end() && (++it) not_eq jar.end()) );

          ++it;
          EXPECT( (it not_eq jar.end() && (++it) == jar.end()) );
        }

        THEN("Get an iterator to the last Cookie")
        {
          auto it = jar.end();
          EXPECT( (it not_eq jar.begin() && (--it) not_eq jar.begin()) );

          --it;
          EXPECT( (it not_eq jar.begin() && (--it) == jar.begin()) );
        }
      }

      WHEN("Inserting one Cookie into the CookieJar")
      {
        jar.insert("name", "value");

        EXPECT(jar.size() == 1u);

        THEN("Get an iterator to the first Cookie")
        {
          auto it = jar.begin();
          EXPECT( (it not_eq jar.end() && (++it) == jar.end()) );
        }

        THEN("Get an iterator to the last Cookie")
        {
          auto it = jar.end();
          EXPECT( (it not_eq jar.begin() && (--it) == jar.begin()) );
        }
      }
    }
  }
};

int main(int argc, char * argv[])
{
  printf("Running CookieJar-tests...\n");

  int res = lest::run(test_cookie_jar, argc, argv);

  printf("CookieJar-tests completed.\n");

  return res;
}
