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

CASE("URI constructor throws on invalid uris") {
  EXPECT_THROWS(uri::URI uri {"?!?!?!"});
}

CASE("scheme() returns the URI's scheme") {
  uri::URI uri {"http://www.vg.no"};
  EXPECT(uri.scheme() == "http");
}

CASE("userinfo() returns the URI's user information") {
    uri::URI uri {"http://Aladdin:OpenSesame@www.vg.no"};
    EXPECT(uri.userinfo() == "Aladdin:OpenSesame");
}

CASE("host() returns the URI's host") {
  uri::URI uri {"http://www.vg.no/"}; //?
  EXPECT(uri.host() == "www.vg.no");
}

CASE("host() returns the URI's host (no path)") {
  uri::URI uri {"http://www.vg.no"};
  EXPECT(uri.host() == "www.vg.no");
}

CASE("port() returns the URI's port") {
  uri::URI uri {"http://www.vg.no:80/"};
  EXPECT(uri.port() == 80);
}

CASE("port() returns the URI's port (no path)") {
  uri::URI uri {"http://www.vg.no:80"};
  EXPECT(uri.port() == 80);
}

CASE("port() defaults according to the correct protocol") {
  uri::URI http {"http://www.vg.no"};
  EXPECT(http.port() == 80);

  uri::URI ftp {"ftp://silkroad.com"};
  EXPECT(ftp.port() == 21);

  uri::URI irc {"irc://irc.includeos.org"};
  EXPECT(irc.port() == 6667);

  uri::URI random {"lul://does.not.work"};
  EXPECT(random.port() == 65535);

  uri::URI port_provided {"http://some.random.site.uk:8080"};
  EXPECT(port_provided.port() == 8080);
}

CASE("out-of-range ports throws exception") {
  EXPECT_THROWS(uri::URI uri {"https://www.vg.no:65539"});
}

CASE("path() returns the URI's path") {
  uri::URI uri {"http://www.digi.no/artikler/nytt-norsk-operativsystem-vekker-oppsikt/319971"};
  EXPECT(uri.path() == "/artikler/nytt-norsk-operativsystem-vekker-oppsikt/319971");
}

CASE("query() returns unparsed query string") {
    uri::URI uri {"http://www.vg.no?fname=patrick&lname=bateman"};
    EXPECT(uri.query() == "fname=patrick&lname=bateman");
}

CASE("query(\"param\") returns value of query parameter named \"param\"") {
  uri::URI uri {"http://www.vg.no?fname=patrick&lname=bateman"};
  EXPECT(uri.query("fname") == "patrick");
}

CASE("query(\"param\") returns missing query parameters as empty string") {
  uri::URI uri {"http://www.vg.no?fname=patrick&lname=bateman"};
  EXPECT(uri.query("phone") == "");
}

CASE("fragment() returns fragment part") {
  uri::URI uri {"https://github.com/includeos/acorn#take-it-for-a-spin"};
  EXPECT(uri.fragment() == "take-it-for-a-spin");
}

CASE("host_is_ip4() returns whether uri's host is an IPv4 address")
{
  uri::URI uri {"http://www.vg.no?fname=patrick&lname=bateman"};
  EXPECT(uri.host_is_ip4() == false);
  (uri.reset() << "http://172.217.16.164/").parse();
  EXPECT(uri.host_is_ip4() == true);
}

CASE("host_is_ip6() returns whether uri's host is an IPv6 address")
{
  uri::URI uri {"http://www.vg.no?fname=patrick&lname=bateman"};
  EXPECT(uri.host_is_ip6() == false);
  (uri.reset() << "http://[1080::8:800:200C:417A]").parse();
  EXPECT(uri.host_is_ip6() == true);
}

CASE("port_str() returns uri's port as string")
{
  uri::URI uri {"http://www.vg.no:8080"};
  EXPECT(uri.port_str() == "8080");
}

CASE("URI construction, assignment")
{
  uri::URI uri1 {"http://www.vg.no:8080"};
  uri::URI uri2{uri1};
  EXPECT(uri1 == uri2);
  uri::URI uri3 = uri2;
  EXPECT(uri3 == uri1);
  uri::URI uri {"http://includeos.org/"};
  EXPECT_NOT(uri == uri1);
  EXPECT_NOT(uri == uri2);
  EXPECT_NOT(uri == uri3);
}
