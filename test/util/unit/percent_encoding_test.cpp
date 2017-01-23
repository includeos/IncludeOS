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
#include <uri>
#include <strings.h> // strcasecmp

CASE("uri::encode() encodes a string")
{
  std::string encoded {uri::encode("The C++ Programming Language (4th Edition)")};
  static const char* expected = "The%20C%2B%2B%20Programming%20Language%20%284th%20Edition%29";
  EXPECT(strcasecmp(encoded.c_str(), expected) == 0);
}

CASE("uri::decode() decodes an URI-encoded string")
{
  std::string decoded {uri::decode("The%20C%2B%2B%20Programming%20Language%20%284th%20Edition%29")};
  EXPECT(decoded == "The C++ Programming Language (4th Edition)");
}

CASE("uri::decode() throws on invalid input when URI_THROW_ON_ERROR is defined")
{
  EXPECT_THROWS_AS(std::string s = uri::decode("%2x%zz"), std::runtime_error);
}
