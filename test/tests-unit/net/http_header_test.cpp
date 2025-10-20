// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2017 Oslo and Akershus University College of Applied Sciences
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
#include <net/http/header.hpp>

CASE("Header::Header() constructs an empty header with capacity 25")
{
  http::Header header;
  EXPECT(header.is_empty() == true);
  EXPECT(header.size() == 0u);
  // try to add more than capacity
  for (int i = 1; i < 30; ++i) {
    header.add_field("X-Something-Something_" + std::to_string(i), "foo");
  }
  EXPECT(header.is_empty() == false);
  EXPECT(header.size() == 25u);
}

CASE("Header::add_field() adds a header field")
{
  http::Header header;
  EXPECT(header.is_empty() == true);
  auto ok = header.add_field("Accept-Charset", "utf-8");
  EXPECT(ok == true);
  EXPECT(header.is_empty() == false);
  EXPECT(header.size() == 1u);
  ok = header.has_field("Accept-Charset");
  EXPECT(ok == true);
}

CASE("Header::has_field() checks if header has a specific field")
{
  http::Header header;
  EXPECT(header.is_empty() == true);
  auto ok = header.add_field("Accept-Charset", "iso-8859-1");
  EXPECT(ok == true);
  ok = header.has_field("Accept");
  EXPECT(ok == false);
  ok = header.has_field("Accept-Charset");
  EXPECT(ok == true);
}

CASE("Header::value() returns value of specific field")
{
  http::Header header;
  EXPECT(header.is_empty() == true);
  auto ok = header.add_field("Accept-Encoding", "gzip");
  EXPECT(ok == true);
  auto val = header.value("Accept-Encoding");
  EXPECT(val == "gzip");
  val = header.value("Accept");
  EXPECT(val == "");
}

CASE("Header::clear() removes all fields")
{
  http::Header header;
  auto ok = header.add_field("Connection", "keep-alive");
  EXPECT(ok == true);
  EXPECT(header.size() == 1u);
  header.clear();
  EXPECT(header.size() == 0u);
}

CASE("Header::erase() removes specified field")
{
  http::Header header;
  header.add_field("Connection", "keep-alive");
  header.add_field("Accept-Encoding", "gzip");
  header.add_field("Accept-Charset", "iso-8859-1");
  EXPECT(header.size() == 3u);
  header.erase("X-Something-Something");
  EXPECT(header.size() == 3u);
  header.erase("Accept-Encoding");
  EXPECT(header.size() == 2u);
  EXPECT(header.has_field("Accept-Encoding") == false);
}

CASE("Header::set_field() sets specified field ")
{
  http::Header header;
  header.set_field("Connection", "keep-alive");
  EXPECT(header.size() == 1);
  EXPECT(header.has_field("Connection") == true);
  auto val = header.value("Connection");
  EXPECT(val == "keep-alive");
  bool res = header.set_field("Connection", "close");
  EXPECT(res == true);
  val = header.value("Connection");
  EXPECT(val == "close");
}

CASE("Headers can be streamed")
{
  http::Header header;
  bool res = header.set_field("Connection", "close");
  std::stringstream ss;
  ss << header;
  EXPECT(ss.str().size() > 15);
}
