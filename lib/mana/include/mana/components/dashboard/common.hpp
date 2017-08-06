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

#pragma once
#ifndef DASHBOARD_COMMON_HPP
#define DASHBOARD_COMMON_HPP

#include <delegate>
#include <mana/attributes/json.hpp> // rapidjson
#include <mana/request.hpp>
#include <mana/response.hpp>

namespace mana {
namespace dashboard {
  using WriteBuffer = rapidjson::StringBuffer;
  using Writer = rapidjson::Writer<WriteBuffer>;
  using Serialize = delegate<void(Writer&)>;
  using RouteCallback = delegate<void(mana::Request_ptr, mana::Response_ptr)>;
}
}

#endif
