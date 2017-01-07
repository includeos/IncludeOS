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
#include <net/http/time.hpp>

using namespace std::string_literals;

CASE("from_time_t() returns time as string")
{
  time_t epoch {0};
  std::string str {http::time::from_time_t(epoch)};
  EXPECT(str != "");
  EXPECT(str.find("1970") != std::string::npos);
}

CASE("to_time_t() returns time_t from string")
{
  auto str = "Tue, 01 Jan 1980 00:00:00 GMT"s;
  time_t time {http::time::to_time_t(str)};
  EXPECT(time != std::time_t{});
}
