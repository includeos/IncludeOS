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
#include <net/http/response.hpp>

using namespace std::string_literals;

CASE("HTTP response can be created from a string")
{
  http::Response r("HTTP/1.1 200 OK\r\nServer: IncludeOS/Acorn\r\nConnection: keep-alive\r\nContent-Type: application/json\r\nContent-Length: 194\r\n\r\n{\"version\":\"v0.9.3-1868-gd008d1a-dirty\",\"service\":\"Acorn Web Appliance\",\"heap_usage\":438272,\"cpu_freq\":2600.381981722683,\"boot_time\":\"2016-12-20T12:29:28Z\",\"current_time\":\"2017-01-10T16:03:53Z\"}");
  EXPECT(r.status_code() == 200);
  EXPECT(r.version().to_string() == "HTTP/1.1");
}

CASE("status_code() returns the status code from a response")
{
  http::Response r("HTTP/1.1 200 OK\r\nServer: IncludeOS/Acorn\r\nConnection: keep-alive\r\nContent-Type: application/json\r\nContent-Length: 194\r\n\r\n{\"version\":\"v0.9.3-1868-gd008d1a-dirty\",\"service\":\"Acorn Web Appliance\",\"heap_usage\":438272,\"cpu_freq\":2600.381981722683,\"boot_time\":\"2016-12-20T12:29:28Z\",\"current_time\":\"2017-01-10T16:03:53Z\"}");
  EXPECT(r.status_code() == 200);
}

CASE("status code can be set")
{
  http::Response r;
  EXPECT(r.status_code() == 200);
  r.set_status_code(http::Not_Found);
  EXPECT(r.status_code() == 404);
}

CASE("version() returns the HTTP version from a response")
{
  http::Response r("HTTP/1.0 200 OK\r\nServer: IncludeOS/Acorn\r\nConnection: keep-alive\r\nContent-Type: application/json\r\nContent-Length: 194\r\n\r\n{\"version\":\"v0.9.3-1868-gd008d1a-dirty\",\"service\":\"Acorn Web Appliance\",\"heap_usage\":438272,\"cpu_freq\":2600.381981722683,\"boot_time\":\"2016-12-20T12:29:28Z\",\"current_time\":\"2017-01-10T16:03:53Z\"}");
  EXPECT_NOT(r.version().to_string() == "HTTP/1.1");
  EXPECT(r.version().to_string() == "HTTP/1.0");
}

CASE("version can be set")
{
  http::Response r;
  r.set_version(http::Version(1, 0));
  EXPECT(r.version().to_string() == "HTTP/1.0");
}

CASE("reset() resets response to default values")
{
  http::Response r("HTTP/1.0 200 OK\r\nServer: IncludeOS/Acorn\r\nConnection: keep-alive\r\nContent-Type: application/json\r\nContent-Length: 194\r\n\r\n{\"version\":\"v0.9.3-1868-gd008d1a-dirty\",\"service\":\"Acorn Web Appliance\",\"heap_usage\":438272,\"cpu_freq\":2600.381981722683,\"boot_time\":\"2016-12-20T12:29:28Z\",\"current_time\":\"2017-01-10T16:03:53Z\"}");
  r.set_status_code(http::Not_Found);
  r.reset();
  EXPECT_NOT(r.version().to_string() == "HTTP/1.1");
  EXPECT(r.status_code() == 200);
}

CASE("to_string returns string representation of request")
{
  http::Response r;
  r.set_version(http::Version(1, 0));
  r.set_status_code(http::Not_Found);
  EXPECT(r.to_string() == "HTTP/1.0 404 Not Found\r\n");
}

CASE("responses can be streamed")
{
  http::Response r;
  r.set_status_code(http::Not_Found);
  std::stringstream ss;
  ss << r;
  EXPECT(ss.str().size() > 20);
}

CASE("status_line() returns the status line portion of the response")
{
  http::Response r("HTTP/1.0 200 OK\r\nServer: IncludeOS/Acorn\r\nConnection: keep-alive\r\nContent-Type: application/json\r\nContent-Length: 194\r\n\r\n{\"version\":\"v0.9.3-1868-gd008d1a-dirty\",\"service\":\"Acorn Web Appliance\",\"heap_usage\":438272,\"cpu_freq\":2600.381981722683,\"boot_time\":\"2016-12-20T12:29:28Z\",\"current_time\":\"2017-01-10T16:03:53Z\"}");
  EXPECT(r.status_line() == "HTTP/1.0 200 OK");
}

CASE("operator std::string returns string representation of response")
{
  http::Response r;
  r.set_version(http::Version(1, 0));
  r.set_status_code(http::Not_Found);
  std::string s = (std::string)r;
  EXPECT(s == "HTTP/1.0 404 Not Found\r\n");
}
