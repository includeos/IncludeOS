// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2016-2017 Oslo and Akershus University College of Applied Sciences
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
#include <net/http/request.hpp>

CASE("A HTTP request can be created from a string")
{
  http::Request r("GET / HTTP/1.1\r\nUser-Agent: IncludeOS/0.9\r\nConnection: close\r\nHost: www.google.com\r\n\r\n");
  EXPECT(r.version().to_string() == "HTTP/1.1");
  EXPECT(http::method::str(r.method()) == "GET");
}

CASE("method() returns the HTTP method for the request")
{
  http::Request r("HEAD / HTTP/1.0\r\nUser-Agent: IncludeOS/0.9\r\nHost: www.google.com\r\n\r\n");
  EXPECT(http::method::str(r.method()) == "HEAD");
}

CASE("HTTP method can be set for the request")
{
  http::Request r("HEAD / HTTP/1.0\r\nUser-Agent: IncludeOS/0.9\r\nHost: www.google.com\r\n\r\n");
  EXPECT(http::method::str(r.method()) == "HEAD");
  r.set_method(http::GET);
  EXPECT(http::method::str(r.method()) == "GET");
}

CASE("uri() returns the uri the request")
{
  http::Request r("GET /intl/en/about/ HTTP/1.0\r\nUser-Agent: IncludeOS/0.9\r\nHost: www.google.com\r\n\r\n");
  const uri::URI about {"/intl/en/about/"};
  EXPECT(r.uri() == about);
}

CASE("uri can be set for the request")
{
  http::Request r("GET /intl/en/about/ HTTP/1.0\r\nUser-Agent: IncludeOS/0.9\r\nHost: www.google.com\r\n\r\n");
  const uri::URI about {"/intl/en/about/"};
  EXPECT(r.uri() == about);
  const uri::URI home {"/"};
  r.set_uri(home);
  EXPECT(r.uri().to_string() == "/");
}

CASE("version() returns the HTTP version of the request")
{
  http::Request r("GET / HTTP/1.0\r\nHost: www.includeos.org\r\n\r\n");
  EXPECT(r.version().to_string() == "HTTP/1.0");
}

CASE("version can be set for the request")
{
  http::Request r("GET / HTTP/1.0\r\nHost: www.includeos.org\r\n\r\n");
  EXPECT(r.version().to_string() == "HTTP/1.0");
  http::Version new_version; // default is 1.1
  r.set_version(new_version);
  EXPECT(r.version().to_string() == "HTTP/1.1");
}

CASE("query_value() returns value for specified query if present")
{
  http::Request r("GET /?q=includeos&t=hs&ia=software HTTP/1.1\r\nHost: duckduckgo.com\r\n\r\n");
  EXPECT(r.query_value("q") == "includeos");
  EXPECT(r.query_value("name") == "");
}

CASE("post_value() returns value from field in post request")
{
  http::Request r("POST /unicorn/register.pl HTTP/1.0\r\nFrom: user@example.com\r\nUser-Agent: wabisabi/1.0\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: 39\r\n\r\ncity=Bellevue&favorite+place=John+Howie");
  EXPECT(http::method::str(r.method()) == "POST");
  EXPECT(r.post_value("city") == "Bellevue");
  EXPECT(r.post_value("not_present") == "");
  EXPECT(r.post_value("") == "");

  // post_value only works for POST requests
  http::Request r2("GET / HTTP/1.0\r\nHost: www.includeos.org\r\n\r\n");
  EXPECT(r2.post_value("not_even_post") == "");
}

CASE("reset() resets request to default values")
{
  http::Request r("HEAD /?test=3 HTTP/1.0\r\nHost: www.includeos.org\r\n\r\n");
  EXPECT(r.version().to_string() == "HTTP/1.0");
  EXPECT(http::method::str(r.method()) == "HEAD");
  r.reset();
  EXPECT(r.version().to_string() == "HTTP/1.1");
  EXPECT(http::method::str(r.method()) == "GET");
  EXPECT(r.uri() == uri::URI("/"));
}

CASE("to_string() returns string representation of request")
{
  http::Request r("GET /");
  r.set_method(http::HEAD);
  r.set_uri(uri::URI("/data/json/includeos_stats"));
  r.set_version(http::Version(1, 0));
  EXPECT(r.to_string() == "HEAD /data/json/includeos_stats HTTP/1.0\r\n");
}

CASE("Requests can be streamed")
{
  http::Request r("GET / HTTP/1.1");
  std::stringstream ss;
  ss << r;
  EXPECT(ss.str().size() > 10u);
}

CASE("make_request makes requests")
{
  auto req_ptr = http::make_request("GET / HTTP/1.1");
  EXPECT(req_ptr->version().to_string() == "HTTP/1.1");
}

CASE("operator std::string converts request to string")
{
  http::Request r("GET /");
  r.set_method(http::HEAD);
  r.set_uri(uri::URI("/data/json/includeos_stats"));
  r.set_version(http::Version(1, 1));
  std::string s = r;
  EXPECT(s.size() > 30);
}
