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

#include <os>
#include <cstdint>

bool verb = true;

#define MYINFO(X,...) INFO("Service", X, ##__VA_ARGS__)

void Service::start(const std::string& args)
{
  MYINFO("Testing kernel modules. Args: %s", args.c_str());

  auto mods = os::modules();

  for (auto mod : mods) {
    INFO2("* %s @ 0x%x - 0x%x, size: %ib",
           reinterpret_cast<char*>(mod.params),
          mod.mod_start, mod.mod_end, mod.mod_end - mod.mod_start);
  }

  CHECKSERT(mods.size() == 1, "Found %zu modules", mods.size());

  printf("SUCCESS\n");
}
