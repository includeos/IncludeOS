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

#include <common.cxx>
#include <net/http/cookie.hpp>

using namespace std;
using namespace http;

  // --------------- Testing Cookie name ------------------

  CASE("CookieException thrown when creating Cookie with no name")
  {
    EXPECT_THROWS_AS( (Cookie{"", "value"}), CookieException );
    EXPECT_THROWS( (Cookie{"", "value"}) );
  }

  CASE("CookieException thrown when creating Cookie with name containing invalid character")
  {
    EXPECT_THROWS( (Cookie{":name", "value"}) );
    EXPECT_THROWS( (Cookie{"(name", "value"}) );
    EXPECT_THROWS( (Cookie{"name)", "value"}) );
    EXPECT_THROWS( (Cookie{"n;ame", "value"}) );
    EXPECT_THROWS( (Cookie{"na[me", "value"}) );
    EXPECT_THROWS( (Cookie{"na]me", "value"}) );
    EXPECT_THROWS( (Cookie{"nam<e", "value"}) );
    EXPECT_THROWS( (Cookie{"name>", "value"}) );
    EXPECT_THROWS( (Cookie{"nam/e", "value"}) );
    EXPECT_THROWS( (Cookie{"n\ame", "value"}) );
    EXPECT_THROWS( (Cookie{"name@", "value"}) );
    EXPECT_THROWS( (Cookie{"na,me", "value"}) );
    EXPECT_THROWS( (Cookie{"n=ame", "value"}) );
    EXPECT_THROWS( (Cookie{"na?me", "value"}) );
    EXPECT_THROWS( (Cookie{"na\nme", "value"}) );
    EXPECT_THROWS( (Cookie{"nam e", "value"}) );
    EXPECT_THROWS( (Cookie{"nam  e", "value"}) );
    EXPECT_THROWS( (Cookie{"n\tame", "value"}) );
    EXPECT_THROWS( (Cookie{"n{ame", "value"}) );
    EXPECT_THROWS( (Cookie{"name}", "value"}) );
    EXPECT_THROWS( (Cookie{":nam[]{e", "value"}) );
  }

  CASE("No CookieException thrown when creating Cookie with valid name")
  {
    EXPECT_NO_THROW( (Cookie{"name", "value"}) );
    EXPECT_NO_THROW( (Cookie{"1234", "value"}) );
    EXPECT_NO_THROW( (Cookie{"'IP412&", "value"}) );

  /* Valid name characters according to the RFC: Any chars excluding
   * control characters and special characters.
   *
   * RFC2068 lists special characters as
   * tspecials      = "(" | ")" | "<" | ">" | "@"
   *                     | "," | ";" | ":" | "\" | <">
   *                     | "/" | "[" | "]" | "?" | "="
   *                     | "{" | "}" | SP | HT
   *
   * SP             = <US-ASCII SP, space (32)>
   * HT             = <US-ASCII HT, horizontal-tab (9)>
   */
  }

  // --------------- Testing Cookie value ------------------

  CASE("No CookieException thrown when creating Cookie with no value")
  {
    EXPECT_NO_THROW( (Cookie{"name", ""}) );
  }

  CASE("CookieException thrown when creating Cookie with value containing invalid character")
  {
    EXPECT_THROWS( (Cookie{"name", ":name"}) );
    EXPECT_THROWS( (Cookie{"name", "(value"}) );
    EXPECT_THROWS( (Cookie{"name", "value)"}) );
    EXPECT_THROWS( (Cookie{"name", "v;alue"}) );
    EXPECT_THROWS( (Cookie{"name", "va[lue"}) );
    EXPECT_THROWS( (Cookie{"name", "val]ue"}) );
    EXPECT_THROWS( (Cookie{"name", "valu<e"}) );
    EXPECT_THROWS( (Cookie{"name", "value>"}) );
    EXPECT_THROWS( (Cookie{"name", "/value"}) );
    EXPECT_THROWS( (Cookie{"name", "v\alue"}) );
    EXPECT_THROWS( (Cookie{"name", "value@"}) );
    EXPECT_THROWS( (Cookie{"name", "val,ue"}) );
    EXPECT_THROWS( (Cookie{"name", "v=alue"}) );
    EXPECT_THROWS( (Cookie{"name", "valu?e"}) );
    EXPECT_THROWS( (Cookie{"name", "valu\e"}) );
    EXPECT_THROWS( (Cookie{"name", "valu e"}) );
    EXPECT_THROWS( (Cookie{"name", "val  ue"}) );
    EXPECT_THROWS( (Cookie{"name", "va\tlue"}) );
    EXPECT_THROWS( (Cookie{"name", "va{lue"}) );
    EXPECT_THROWS( (Cookie{"name", "value}"}) );
    EXPECT_THROWS( (Cookie{"name", "v:a[]l{ue"}) );
  }

  CASE("No CookieException thrown when creating Cookie with valid value")
  {
    EXPECT_NO_THROW( (Cookie{"name", "value"}) );
    EXPECT_NO_THROW( (Cookie{"name", "1928sdfg'"}) );
    EXPECT_NO_THROW( (Cookie{"name", "&INAann'dp21"}) );
  }

  // --------------- Testing Cookie options ------------------

  // Option vector in general

  // Cases that throw CookieExceptions

  CASE("CookieException thrown when creating Cookie with empty option name")
  {
    EXPECT_THROWS( (Cookie{"name", "value", {"", "Sun, 06 Nov 1994 08:49:37 GMT"}}) );
  }

  CASE("CookieException thrown when creating Cookie with invalid option name")
  {
    EXPECT_THROWS( (Cookie{"name", "value", {"something", "something"}}) );
    EXPECT_THROWS( (Cookie{"name", "value", {"expires_", "Sun, 06 Nov 1994 08:49:37 GMT"}}) );
  }

  CASE("CookieException thrown when creating Cookie with odd number of vector-elements")
  {
    EXPECT_THROWS( (Cookie{"name", "value", {"Expires", "Sun, 06 Nov 1994 08:49:37 GMT", "Path"}}) );
    EXPECT_THROWS( (Cookie{"name", "value", {"Expires"}}) );
    EXPECT_THROWS( (Cookie{"name", "value", {"Expires", "Sun, 06 Nov 1994 08:49:37 GMT", "Path", "/something123", "Domain"}}) );
  }

  CASE("No CookieException thrown when creating Cookie with multiple valid options")
  {
    EXPECT_NO_THROW( (Cookie{"name", "value", {"Expires", "Sun, 06 Nov 1994 08:49:37 GMT", "path", "/path"}}) );
    EXPECT_NO_THROW( (Cookie{"name", "value", {"Expires", "Sun, 06 Nov 1994 08:49:37 GMT", "path", ""}}) );
    EXPECT_NO_THROW( (Cookie{"name", "value", {"Expires", "Sun, 06 Nov 1994 08:49:37 GMT", "Path", "/some_path_here",
      "domain", "example.com"}}) );
    EXPECT_NO_THROW( (Cookie{"name", "value", {"Expires", "Sun, 06 Nov 1994 08:49:37 GMT", "Path", "/some_path_here",
      "domain", "example.com", "secure", "true", "httponly", "true"}}) );
    EXPECT_NO_THROW( (Cookie{"name", "value", {"Expires", "Sun, 06 Nov 1994 08:49:37 GMT", "Path", "/anotherPath134",
      "secure", "true", "domain", "example.com"}}) );
  }

  // Scenario when creating cookie. Check the set values.

  CASE("Creating cookies with valid values and changing these values")
  {
    GIVEN("An option vector with all possible options sent to Cookie constructor")
    {
      /* Previously had max-age as a std::chrono::seconds object:
      chrono::seconds s{100};
      ostringstream stream;
      stream << s.count();
      ... Cumbersome for the developer.
      RATHER: Set max-age from int instead of chrono::seconds.
      Then in cookie constructor: convert from string to int
      */

      /* Better for the developer:
      int seconds = 10*60;
      string s = to_string(seconds);
      printf("Number of seconds string: %s\n", s.c_str());
      vector<string> v{"Expires", "Sun, 06 Nov 1994 08:49:37 GMT", "Path", "/anotherPath134",
          "secure", "true", "httponly", "true", "Domain", "example.com", "Max-Age", s};
      Or just: */
      vector<string> v{"Expires", "Sun, 06 Nov 2004 08:49:37 GMT", "Path", "/anotherPath134",
          "secure", "true", "httponly", "true", "Domain", "example.com", "Max-Age", "600"};

      WHEN("Creating cookie")
      {
        Cookie c{"name", "value", v};

        THEN("Get the input values back")
        {
          EXPECT(c.get_name() == "name");
          EXPECT(c.get_value() == "value");
          EXPECT(c.get_expires() == "Sun, 06 Nov 2004 08:49:37 GMT");
          EXPECT(c.get_path() == "/anotherPath134");
          EXPECT(c.get_domain() == "example.com");
          EXPECT(c.get_max_age() == 600);
          EXPECT(c.is_secure());
          EXPECT(c.is_http_only());
        }

        AND_THEN("Get a string containing existing values when calling the cookie's to_string method")
        {
          EXPECT(c.to_string() == "name=value; Expires=Sun, 06 Nov 2004 08:49:37 GMT; Max-Age=600; Domain=example.com; Path=/anotherPath134; Secure; HttpOnly");
        }
      }
    }

    GIVEN("A Cookie object just containing name and value")
    {
      Cookie c{"name", "value"};

      WHEN("Accessing all cookie attributes")
      {
        THEN("Get the default cookie options")
        {
          EXPECT(c.get_name() == "name");
          EXPECT(c.get_value() == "value");
          EXPECT(c.get_path() == "/");
          EXPECT(c.get_domain() == "");
          EXPECT(c.get_max_age() == -1);
          EXPECT(c.get_expires() == "");
          EXPECT_NOT(c.is_secure());
          EXPECT_NOT(c.is_http_only());
        }
      }

      WHEN("Getting the cookie's to_string")
      {
        THEN("It only contains existing values")
        {
          EXPECT( c.to_string() == "name=value; Path=/" );
        }
      }

      WHEN("Setting all cookie attributes to valid values")
      {
        c.set_value("new_value");
        c.set_path("/a_path");
        c.set_domain(".DOMAIN.no");
        c.set_max_age(1000);
        c.set_expires("Tue, 23 Jul 2016 13:59:40 GMT");
        c.set_secure(true);
        c.set_http_only(true);

        THEN("Get the set values")
        {
          EXPECT(c.get_value() == "new_value");
          EXPECT(c.get_path() == "/a_path");
          EXPECT(c.get_domain() == "domain.no");
          EXPECT(c.get_max_age() == 1000);
          EXPECT(c.get_expires() == "Tue, 23 Jul 2016 13:59:40 GMT");
          EXPECT(c.is_secure());
          EXPECT(c.is_http_only());
        }
      }

      WHEN("Setting cookie attribute Max-Age to invalid value")
      {
        EXPECT_THROWS(c.set_max_age(-1));
      }
    }

    GIVEN("A Cookie object containing name, value and two valid options")
    {
      Cookie c{"name", "value", {"Max-Age", "402000", "Path", "/some path"}};

      WHEN("Accessing all cookie attributes")
      {
        THEN("The specified values have been set")
        {
          EXPECT(c.get_name() == "name");
          EXPECT(c.get_value() == "value");
          EXPECT(c.get_max_age() == 402000);
          EXPECT(c.get_path() == "/some path");
        }
      }

      WHEN("Getting the cookie's to_string")
      {
        THEN("It only contains existing values")
        {
          EXPECT(c.to_string() == "name=value; Max-Age=402000; Path=/some path");
        }
      }
    }
  }

  // Option: Expires (string GMT datetime)

  CASE("No CookieException thrown when creating Cookie with valid Expires option")
  {
    // Format: Sun, 06 Nov 1994 08:49:37 GMT / %a, %d %b %Y %H:%M:%S %Z

    EXPECT_NO_THROW( (Cookie{"name", "value", {"Expires", "Sun, 06 Nov 1994 08:49:37 GMT"}}) );
    EXPECT_NO_THROW( (Cookie{"name", "value", {"expires", "Mon, 12 Jan 1995 23:59:00 GMT"}}) );
    EXPECT_NO_THROW( (Cookie{"name", "value", {"Expires", "tue, 30 May 1999 21:09:55 GMT"}}) );
    EXPECT_NO_THROW( (Cookie{"name", "value", {"Expires", "Wed, 31 Dec 2018 16:09:59 GMT"}}) );
    EXPECT_NO_THROW( (Cookie{"name", "value", {"expires", "Thu, 22 Feb 2013 19:17:11 GMT"}}) );
    EXPECT_NO_THROW( (Cookie{"name", "value", {"Expires", "fri, 01 Mar 2023 13:00:06 GMT"}}) );
    EXPECT_NO_THROW( (Cookie{"name", "value", {"expiREs", "saT, 9 Apr 2016 00:09:44 GMT"}}) );

    // Format: Sunday, 06-Nov-94 08:49:37 GMT / %a, %d-%b-%y %H:%M:%S %Z

    EXPECT_NO_THROW( (Cookie{"name", "value", {"Expires", "Sunday, 06-Nov-94 08:49:37 GMT"}}) );
    EXPECT_NO_THROW( (Cookie{"name", "value", {"expires", "Monday, 12-Jan-95 23:59:00 GMT"}}) );
    EXPECT_NO_THROW( (Cookie{"name", "value", {"Expires", "tuesday, 30-May-99 21:09:55 GMT"}}) );
    EXPECT_NO_THROW( (Cookie{"name", "value", {"Expires", "Wednesday, 31-Dec-18 16:09:59 GMT"}}) );
    EXPECT_NO_THROW( (Cookie{"name", "value", {"expires", "Thursday, 22-Feb-13 19:17:11 GMT"}}) );
    EXPECT_NO_THROW( (Cookie{"name", "value", {"Expires", "friday, 01-Mar-23 13:00:06 GMT"}}) );
    EXPECT_NO_THROW( (Cookie{"name", "value", {"expIres", "saTurday, 09-Apr-16 00:09:44 GMT"}}) );

    // Format: Sun Nov 6 08:49:37 1994 / %a %b %d %H:%M:%S %Y

    EXPECT_NO_THROW( (Cookie{"name", "value", {"Expires", "Sun Nov 6 08:49:37 1994"}}) );
    EXPECT_NO_THROW( (Cookie{"name", "value", {"expires", "Mon Jan 12 23:59:00 1995"}}) );
    EXPECT_NO_THROW( (Cookie{"name", "value", {"Expires", "tue May 30 21:09:55 1999"}}) );
    EXPECT_NO_THROW( (Cookie{"name", "value", {"Expires", "Wed Dec 31 16:09:59 2018"}}) );
    EXPECT_NO_THROW( (Cookie{"name", "value", {"expires", "Thu Feb 22 19:17:11 2013"}}) );
    EXPECT_NO_THROW( (Cookie{"name", "value", {"Expires", "fri Mar 1 13:00:06 2023"}}) );
    EXPECT_NO_THROW( (Cookie{"name", "value", {"expiRes", "saT Apr 9 00:09:44 2016"}}) );
  }

  CASE("CookieException thrown when creating Cookie with invalid Expires option")
  {
    // Format: Sun, 06 Nov 1994 08:49:37 GMT / %a, %d %b %Y %H:%M:%S %Z

    EXPECT_THROWS( (Cookie{"name", "value", {"Expires", "Sunday 06 Nov 1994 08:49:37 GMT"}}) );
    EXPECT_THROWS( (Cookie{"name", "value", {"Expires", "Man, 12 Jan 1995 23:59:00 GMT"}}) );
    EXPECT_THROWS( (Cookie{"name", "value", {"Expires", "tue, 30-May 1999 21:09:55 GMT"}}) );
    EXPECT_THROWS( (Cookie{"name", "value", {"Expires", "Wed, 31 Des 2018 16:09:59 GMT"}}) );
    EXPECT_THROWS( (Cookie{"name", "value", {"Expires", "Thu, 22 Feb 2013 24:17:11 GMT"}}) );
    EXPECT_THROWS( (Cookie{"name", "value", {"Expires", "fri, 01 Mar 2023 25:00:06 GMT"}}) );
    EXPECT_THROWS( (Cookie{"name", "value", {"Expires", "saT 09 Apr 2016 00:09:44 GMT"}}) );
    EXPECT_THROWS( (Cookie{"name", "value", {"Expires", "saT, 09 Jun-99 00:09:44 GMT"}}) );
    EXPECT_THROWS( (Cookie{"name", "value", {"Expires", "abc"}}) );
    EXPECT_THROWS( (Cookie{"name", "value", {"Expires", "saT, Apr 16 00:09:44 GMT"}}) );
    #ifndef __APPLE__
    // this one broken on macaroni
    EXPECT_THROWS( (Cookie{"name", "value", {"Expires", "Sun Nov 6 08:49:37"}}) );
    #endif
    EXPECT_THROWS( (Cookie{"name", "value", {"Expires", ""}}) );
  }

  // Option: Max-Age (int)

  CASE("No CookieException thrown when creating Cookie with valid Max-Age option")
  {
    EXPECT_NO_THROW( (Cookie{"name", "value", {"Max-Age", "800"}}) );
    EXPECT_NO_THROW( (Cookie{"name", "value", {"Max-AgE", "0"}}) );
    EXPECT_NO_THROW( (Cookie{"name", "value", {"MAX-Age", "100000"}}) );
    EXPECT_NO_THROW( (Cookie{"name", "value", {"max-age", "5011010"}}) );
  }

  CASE("CookieException thrown when creating Cookie with invalid Max-Age option")
  {
    EXPECT_THROWS( (Cookie{"name", "value", {"Max-Age", "-1"}}) );
    EXPECT_THROWS( (Cookie{"name", "value", {"Max-Age", "ab"}}) );
    EXPECT_THROWS( (Cookie{"name", "value", {"Max-Age", ";12"}}) );
    EXPECT_THROWS( (Cookie{"name", "value", {"Max-Age", "9999999999"}}) );
  }

  // Option: Domain (string)

  CASE("No CookieException thrown when creating Cookie with valid Domain option")
  {
    EXPECT_NO_THROW( (Cookie{"name", "value", {"domain", ".domain.com"}}) );
    EXPECT_NO_THROW( (Cookie{"name", "value", {"Domain", "example.com"}}) );
    EXPECT_NO_THROW( (Cookie{"name", "value", {"DOMAIN", "service.example.com"}}) );
    EXPECT_NO_THROW( (Cookie{"name", "value", {"Domain", "whateverdomainyouwant123.thebrowserremovesitifnotyourdomain"}}) );
    EXPECT_NO_THROW( (Cookie{"name", "value", {"Domain", ""}}) );
  }

  /* All characters are allowed in the domain option, but the browser will ignore it if the domain you enter isn't yours
  CASE("CookieException thrown when creating Cookie with invalid Domain option") {},
  */

  // Option: Path (string)

  CASE("No CookieException thrown when creating Cookie with valid Path option")
  {
    EXPECT_NO_THROW( (Cookie{"name", "value", {"path", "/my_path"}}) );
    EXPECT_NO_THROW( (Cookie{"name", "value", {"Path", "/example132"}}) );
    EXPECT_NO_THROW( (Cookie{"name", "value", {"PATH", "/ex?mypath even with whitespaces"}}) );
    EXPECT_NO_THROW( (Cookie{"name", "value", {"Path", "/whatever/path/you/want/:&something"}}) );
    EXPECT_NO_THROW( (Cookie{"name", "value", {"Path", ""}}) );
  }

  CASE("Path is set")
  {
    Cookie c{"name", "value", {"path", "/my_path"}};
    Cookie c2{"name", "value", {"Path", "/example132"}};
    Cookie c3{"name", "value", {"PATH", "/ex?mypath even with whitespaces"}};
    Cookie c4{"name", "value", {"Path", "/whatever/path/you/want/:&something"}};
    Cookie c5{"name", "value", {"Path", ""}};

    EXPECT( c.get_path() == "/my_path" );
    EXPECT( c2.get_path() == "/example132" );
    EXPECT( c3.get_path() == "/ex?mypath even with whitespaces" );
    EXPECT( c4.get_path() == "/whatever/path/you/want/:&something" );
    EXPECT( c5.get_path() == "/" );
  }

  CASE("CookieException thrown when creating Cookie with invalid Path option")
  {
    EXPECT_THROWS( (Cookie{"name", "value", {"Path", "/;invalidpath"}}) );
    EXPECT_THROWS( (Cookie{"name", "value", {"Path", "invalid"}}) );
    EXPECT_THROWS( (Cookie{"name", "value", {"Path", "/12\rmyPath"}}) );
    EXPECT_THROWS( (Cookie{"name", "value", {"Path", "/12\nmyPath"}}) );
    EXPECT_THROWS( (Cookie{"name", "value", {"Path", "/12\tmyPath"}}) );
  }

  // Option: Secure (bool)

  CASE("Correct bool value set when creating Cookie with valid Secure option")
  {
    Cookie c{"name", "value", {"Secure", "true"}};
    Cookie c2{"name", "value", {"Secure", "false"}};
    Cookie c3{"name", "value", {"SeCure", "TRUE"}};
    Cookie c4{"name", "value", {"secure", "FALSE"}};
    Cookie c5{"name", "value", {"Secure", "not_valid"}};

    EXPECT( c.is_secure() );
    EXPECT_NOT( c2.is_secure() );
    EXPECT( c3.is_secure() );
    EXPECT_NOT( c4.is_secure() );
    EXPECT_NOT( c5.is_secure() );
  }

  // Option: HttpOnly (bool)

  CASE("Correct bool value set when creating Cookie with invalid HttpOnly option")
  {
    Cookie c{"name", "value", {"httponly", "true"}};
    Cookie c2{"name", "value", {"HttpOnly", "false"}};
    Cookie c3{"name", "value", {"HttPONLY", "TRUE"}};
    Cookie c4{"name", "value", {"httponly", "FALSE"}};
    Cookie c5{"name", "value", {"HttpOnly", "not_valid"}};

    EXPECT( c.is_http_only() );
    EXPECT_NOT( c2.is_http_only() );
    EXPECT( c3.is_http_only() );
    EXPECT_NOT( c4.is_http_only() );
    EXPECT_NOT( c5.is_http_only() );
  }

CASE("Cookies can be streamed")
{
  Cookie c{"name", "value"};
  std::stringstream ss;
  ss << c;
  EXPECT(ss.str().size() > 13);
}

CASE("CookieException::what() returns string describing exception")
{
  try {
    Cookie c {"name", "value", {"Path", "/;invalidpath"}};
  }
  catch (const CookieException& ce) {
    const auto msg_len = strlen(ce.what());
    EXPECT(msg_len > 20);
  }
}

CASE("Cookie::set_value throws if attempting to set invalid value")
{
  Cookie c {"name", "value"};
  EXPECT_THROWS(c.set_value("v:a[]l{ue____"));
}
