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

#include <common.cxx>
#include <net/addr.hpp>

using namespace net;

CASE("Addr")
{
  Addr addr{};

  ip4::Addr ip4addr;
  ip6::Addr ip6addr;

  EXPECT(addr.is_any());
  //EXPECT(addr.v4() == ip4addr);
  EXPECT(addr.v6() == ip6addr);

  addr.set_v4({10,0,0,42});
  printf("%s\n", addr.v4().to_string().c_str());
  printf("%s\n", addr.v6().to_string().c_str());

}
