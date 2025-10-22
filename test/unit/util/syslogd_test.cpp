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
#include <syslog.h>
#include <util/syslogd.hpp>

CASE("valid_priority() returns whether supplied priority is valid")
{
  EXPECT(Syslog::valid_priority(LOG_EMERG) == true);
  EXPECT(Syslog::valid_priority(LOG_DEBUG) == true);
  EXPECT_NOT(Syslog::valid_priority(8192007) == true);
}

CASE("valid_logopt() returns whether supplied logopt is valid")
{
  EXPECT(Syslog::valid_logopt(LOG_PID || LOG_NOWAIT) == true);
}

CASE("valid_facility() returns whether supplied facility is valid")
{
  EXPECT(Syslog::valid_facility(LOG_USER) == true);
}

