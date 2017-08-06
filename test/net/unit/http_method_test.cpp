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
#include <net/http/methods.hpp>

CASE("is_content_length_required() returns whether content length is required")
{
  const http::Method get(http::GET);
  const http::Method post(http::POST);
  EXPECT(http::method::is_content_length_required(get) == false);
  EXPECT(http::method::is_content_length_required(post) == true);
}

CASE("is_content_length_allowed() returns whether content length is allowed")
{
  const http::Method get(http::GET);
  const http::Method post(http::POST);
  const http::Method put(http::PUT);
  EXPECT(http::method::is_content_length_allowed(get) == false);
  EXPECT(http::method::is_content_length_allowed(post) == true);
  EXPECT(http::method::is_content_length_allowed(put) == true);
}

CASE("str() returns string representation of method")
{
  const http::Method m(http::GET);
  EXPECT(http::method::str(m) == "GET");
}

CASE("method() returns method as specified by string")
{
  const http::Method m1(http::method::code("GET"));
  EXPECT(http::method::is_content_length_required(m1) == false);
  const http::Method m2(http::method::code("TELEPORT"));
  EXPECT(http::method::str(m2) == "INVALID");
}
