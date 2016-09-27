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

#ifndef SERVER_ROUTE_HPP
#define SERVER_ROUTE_HPP

#include <regex>
#include <string>
#include <delegate>

#include "request.hpp"
#include "response.hpp"
#include "../route/path_to_regex.hpp"

namespace server {

using End_point  = delegate<void(Request_ptr, Response_ptr)>;
using Route_expr = std::regex;

struct Route {
  Route(const std::string& ex, End_point e)
    : path{ex}
    , end_point{e}
  {
    expr = route::Path_to_regex::path_to_regex(path, keys);
  }

  std::string path;
  Route_expr  expr;
  End_point   end_point;
  route::Keys keys;
  size_t      hits {0U};
}; //< struct Route

inline bool operator < (const Route& lhs, const Route& rhs) noexcept {
  return rhs.hits < lhs.hits;
}

} //< namespace server

#endif //< SERVER_ROUTE_HPP
