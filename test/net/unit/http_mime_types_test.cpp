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
#include <net/http/mime_types.hpp>

CASE("ext_to_mime_type() returns MIME type for specific extension")
{
  EXPECT(http::ext_to_mime_type("html") == "text/html");
  EXPECT(http::ext_to_mime_type("txt") == "text/plain");
  EXPECT(http::ext_to_mime_type("bin") == "application/octet-stream");
}

CASE("ext_to_mime_type() returns 'application/octet-stream' type for unknown extensions")
{
  EXPECT_NO_THROW(http::ext_to_mime_type(""));
  EXPECT(http::ext_to_mime_type("") == "application/octet-stream");
}
