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
#include <os.hpp>
#include <kernel/memory.hpp>

CASE("version() returns string representation of OS version")
{
  EXPECT(os::version() != nullptr);
  EXPECT(std::string(os::version()).size() > 0);
  EXPECT(os::version()[0] == 'v');
  EXPECT(os::arch() != nullptr);
  EXPECT(std::string(os::arch()).size() > 0);
}

CASE("cycles_since_boot() returns clock cycles since boot")
{
  EXPECT(os::cycles_since_boot() != 0ull);
}

CASE("page_size() returns page size")
{
  EXPECT(os::mem::min_psize() == 4096u);
}
