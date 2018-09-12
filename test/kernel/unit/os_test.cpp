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
#include <kernel/os.hpp>

CASE("version() returns string representation of OS version")
{
  EXPECT(OS::version() != nullptr);
  EXPECT(std::string(OS::version()).size() > 0);
  EXPECT(OS::version()[0] == 'v');
  EXPECT(OS::arch() != nullptr);
  EXPECT(std::string(OS::arch()).size() > 0);
}

CASE("cycles_since_boot() returns clock cycles since boot")
{
  EXPECT(OS::cycles_since_boot() != 0ull);
}

CASE("page_size() returns page size")
{
  EXPECT(OS::page_size() == 4096u);
}

CASE("page_nr_from_addr() returns page number from address")
{
  EXPECT(OS::addr_to_page(512) == 0u);
  EXPECT(OS::page_to_addr(1) > 0u);
}
