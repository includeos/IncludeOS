// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017 Oslo and Akershus University College of Applied Sciences
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
#include <isotime>

CASE("A timestamp can be converted into a ISO UTC datetime string")
{
  uint64_t beginning = 0;
  auto str = isotime::to_datetime_string(beginning);

  EXPECT(str == "1970-01-01T00:00:00Z");

  uint64_t one_minute_later = 60;
  str = isotime::to_datetime_string(one_minute_later);

  EXPECT(str == "1970-01-01T00:01:00Z");

  uint64_t five_hours_later = 3600*5;
  str = isotime::to_datetime_string(five_hours_later);

  EXPECT(str == "1970-01-01T05:00:00Z");

  uint64_t big_number = 253402297200L;

  str = isotime::to_datetime_string(big_number);

  EXPECT(str == "9999-12-31T23:00:00Z");

  uint64_t bigger_number = big_number + 2*60*60;

  str = isotime::to_datetime_string(bigger_number);

  // buffer is not big enough for more than 21 chars
  EXPECT(str == "");
}
