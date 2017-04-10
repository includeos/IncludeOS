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

#include <common.cxx>
#include <nic_mock.hpp>
#include <net/inet4.hpp>

using namespace net;

CASE("Creating an ICMP_error of type Too Big")
{
  ICMP_error err{icmp4::Type::DEST_UNREACHABLE, (uint8_t) icmp4::code::Dest_unreachable::FRAGMENTATION_NEEDED, 500};

  EXPECT(err.is_icmp());
  EXPECT(err.icmp_type() == icmp4::Type::DEST_UNREACHABLE);
  EXPECT(err.icmp_code() == (uint8_t) icmp4::code::Dest_unreachable::FRAGMENTATION_NEEDED);
  EXPECT(err.is_too_big());
  EXPECT(err.pmtu() == 500);
}
