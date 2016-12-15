// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2016 Oslo and Akershus University College of Applied Sciences
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
#include <uri>

using namespace std::string_literals;

    CASE("Bogus URI is invalid") {
      uri::URI uri {"?!?!?!"s};
      EXPECT(uri.is_valid() == false);
    }

    CASE("scheme() returns the URI's scheme") {
      uri::URI uri {"http://www.vg.no"s};
      EXPECT(uri.scheme() == "http"s);
    }

    CASE("userinfo() returns the URI's user information") {
        uri::URI uri {"http://Aladdin:OpenSesame@www.vg.no"s};
        EXPECT(uri.userinfo() == "Aladdin:OpenSesame@"s);
    }

    CASE("host() returns the URI's host") {
      uri::URI uri {"http://www.vg.no/"s}; //?
      EXPECT(uri.host() == "www.vg.no"s);
    }

    CASE("[.expectedfailure] host() returns the URI's host (no path)") {
      uri::URI uri {"http://www.vg.no"s};
      EXPECT(uri.host() == "www.vg.no"s);
    }

    CASE("port() returns the URI's port") {
      uri::URI uri {"http://www.vg.no:80/"s};
      EXPECT(uri.port() == 80);
    }

    CASE("[.expectedfailure] port() returns the URI's port (no path)") {
      uri::URI uri {"http://www.vg.no:80"s};
      EXPECT(uri.port() == 80);
    }

    CASE("[.expectedfailure] Out-of-range ports are detected as invalid") {
      uri::URI uri {"http://www.vg.no:65539"s};
      EXPECT(uri.is_valid() == false);
    }

    CASE("path() returns the URI's port") {
      uri::URI uri {"http://www.digi.no/artikler/nytt-norsk-operativsystem-vekker-oppsikt/319971"s};
      EXPECT(uri.path() == "/artikler/nytt-norsk-operativsystem-vekker-oppsikt/319971");
    }

    CASE("query() returns unparsed query string") {
        uri::URI uri {"http://www.vg.no?fname=patrick&lname=bateman"s};
        EXPECT(uri.query() == "?fname=patrick&lname=bateman"s);
    }

    CASE("query(\"param\") returns value of query parameter named \"param\"") {
      uri::URI uri {"http://www.vg.no?fname=patrick&lname=bateman"s};
      EXPECT(uri.query("fname") == "patrick"s);
    }

    CASE("query(\"param\") returns missing query parameters as empty string") {
      uri::URI uri {"http://www.vg.no?fname=patrick&lname=bateman"s};
      EXPECT(uri.query("phone") == ""s);
    }

    CASE("fragment() returns fragment part") {
      uri::URI uri {"https://github.com/includeos/acorn#take-it-for-a-spin"s};
      EXPECT(uri.fragment() == "#take-it-for-a-spin"s);
    }

    CASE("[.expectedfailure] Invalid port does not crash the parser") {
      uri::URI uri {"http://www.vg.no:80999999999999999999999999999"s};
      int port {0};
      EXPECT_NO_THROW(port = uri.port());
    }
