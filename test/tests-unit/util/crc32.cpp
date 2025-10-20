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
#include <util/crc32.hpp>


CASE("Various text strings with known CRC32 matches")
{
  std::string q1 = "... or paste text here ......f";
  uint32_t    a1 = 0x87bbc094;
  
  std::string q2 = "This is a really really really really really really "
                   "really really really really really really really "
                   "really really really really really really really "
                   "really really really really really really really "
                   "really really really really really really really "
                   "really really really really long string.";
  uint32_t    a2 = 0x2eb5c684;
  
  EXPECT(crc32(q1.c_str(), q1.size()) == a1);
  EXPECT(crc32(q2.c_str(), q2.size()) == a2);
  
  // Intel CRC32-C
  EXPECT(crc32_fast(q1.c_str(), q1.size()) == crc32c(q1.c_str(), q1.size()));
  EXPECT(crc32_fast(q2.c_str(), q2.size()) == crc32c(q2.c_str(), q2.size()));
  
}
