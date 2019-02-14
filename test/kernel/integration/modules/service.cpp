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
#include <util/elf_binary.hpp>
#include <util/sha1.hpp>

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

  CHECKSERT(mods.size() == 2, "Found %zu modules", mods.size());

  // Verify module cmdlines
  CHECKSERT(std::string((char*) mods[0].params) == "../mod1.json", "First is mod1.json");
  CHECKSERT(std::string((char*) mods[1].params) == "../mod3.json", "Second is mod3.json");

  // verify content of text modules
  CHECKSERT(std::string((char*) mods[0].mod_start)
          == "{\"module1\" : \"JSON data\" }\n",
          "First JSON has correct content");

  CHECKSERT(std::string((char*) mods[1].mod_start)
          == "{\"module3\" : \"More JSON data, for mod2 service\" }\n",
          "Second JSON has correct content");

  printf("SUCCESS\n");
}
