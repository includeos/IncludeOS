// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2018 IncludeOS AS, Oslo, Norway
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

#include "uplink.hpp"
#include "common.hpp"

namespace uplink {

  static WS_uplink setup_uplink()
  {
    MYINFO("Setting up WS uplink");
    try
    {
      auto config = Config::read();
      return WS_uplink{std::move(config)};
    }
    catch(const std::exception& e)
    {
      MYINFO("Uplink initialization failed: %s ", e.what());
      throw;
    }
  }

  WS_uplink& get()
  {
    static WS_uplink instance{setup_uplink()};
    return instance;
  }

}
