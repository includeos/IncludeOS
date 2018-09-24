// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
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

#include <service>
extern "C" {
  #include "lua.h"
  #include "lualib.h"
  #include "lauxlib.h"
}

const std::string lua_script = R"N0TLU4(

  mytable = setmetatable({key1 = "value1"}, {
     __index = function(mytable, key)

        if key == "key2" then
           return "metatablevalue"
        else
           return mytable[key]
        end
     end
  })

  print(mytable.key1,mytable.key2)

)N0TLU4";

void Service::start()
{
  // initialization
  lua_State * L = luaL_newstate();
  luaL_openlibs(L);

  // execute script
  int load_stat = luaL_loadbuffer(L,lua_script.c_str(), lua_script.size(), "test");
  lua_pcall(L, 0, 0, 0);

  // cleanup
  lua_close(L);
}
