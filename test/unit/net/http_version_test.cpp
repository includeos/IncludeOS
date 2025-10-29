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
#include <net/http/version.hpp>

CASE("Version constructs new HTTP Version, defaulting to 1.1")
{
  http::Version version;
  EXPECT(version.major() == 1u);
  EXPECT(version.minor() == 1u);
  auto s = version.to_string();
  EXPECT(s != "");
  EXPECT(s == "HTTP/1.1");
}

CASE("Versions can be compared")
{
  http::Version v0_9(0, 9);
  http::Version v1_0(1, 0);
  http::Version v1_1;
  http::Version v2_0(2, 0);

  // test ==
  EXPECT((v1_0 == v1_0) == true);
  EXPECT((v1_0 == v2_0) == false);

  // test >
  EXPECT((v0_9 > v0_9) == false);
  EXPECT((v1_0 > v0_9) == true);
  EXPECT((v1_1 > v0_9) == true);
  EXPECT((v2_0 > v0_9) == true);

  EXPECT((v0_9 > v1_0) == false);
  EXPECT((v1_0 > v1_0) == false);
  EXPECT((v1_1 > v1_0) == true);
  EXPECT((v2_0 > v1_0) == true);

  EXPECT((v0_9 > v1_1) == false);
  EXPECT((v1_0 > v1_1) == false);
  EXPECT((v1_1 > v1_1) == false);
  EXPECT((v2_0 > v1_1) == true);

  EXPECT((v0_9 > v2_0) == false);
  EXPECT((v1_0 > v2_0) == false);
  EXPECT((v1_1 > v2_0) == false);
  EXPECT((v2_0 > v2_0) == false);

  // test <
  EXPECT((v0_9 < v0_9) == false);
  EXPECT((v1_0 < v0_9) == false);
  EXPECT((v1_1 < v0_9) == false);
  EXPECT((v2_0 < v0_9) == false);

  EXPECT((v0_9 < v1_0) == true);
  EXPECT((v1_0 < v1_0) == false);
  EXPECT((v1_1 < v1_0) == false);
  EXPECT((v2_0 < v1_0) == false);

  EXPECT((v0_9 < v1_1) == true);
  EXPECT((v1_0 < v1_1) == true);
  EXPECT((v1_1 < v1_1) == false);
  EXPECT((v2_0 < v1_1) == false);

  EXPECT((v0_9 < v2_0) == true);
  EXPECT((v1_0 < v2_0) == true);
  EXPECT((v1_1 < v2_0) == true);
  EXPECT((v2_0 < v2_0) == false);

  // test <= and >=
  EXPECT((v0_9 <= v0_9) == true);
  EXPECT((v1_1 <= v1_0) == false);
  EXPECT((v1_1 <= v2_0) == true);
  EXPECT((v2_0 <= v1_1) == false);
  EXPECT((v2_0 >= v2_0) == true);
  EXPECT((v1_0 >= v2_0) == false);

  // test !=
  EXPECT((v1_0 != v2_0) == true);
  EXPECT((v1_1 != v1_1) == false);

  // test some made-up versions
  http::Version v3_9(3, 9);
  http::Version v5_0(5, 0);

  EXPECT((v5_0 > v3_9) == true);
  EXPECT((v3_9 > v5_0) == false);
  EXPECT((v5_0 < v3_9) == false);
  EXPECT((v3_9 < v5_0) == true);
}

CASE("major() returns major HTTP version number")
{
  http::Version version;
  EXPECT(version.major() == 1u);
}

CASE("major version can be set")
{
  http::Version version(2, 1);
  EXPECT(version.major() == 2u);
  version.set_major(1);
  EXPECT(version.major() == 1u);
}

CASE("minor() returns minor HTTP version number")
{
  http::Version version;
  EXPECT(version.minor() == 1u);
}

CASE("minor version can be set")
{
  http::Version version;
  EXPECT(version.minor() == 1u);
  version.set_minor(0);
  EXPECT(version.minor() == 0u);
}
