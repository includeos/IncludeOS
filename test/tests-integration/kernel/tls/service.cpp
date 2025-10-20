// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
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
#include <cassert>

// TBSS area
thread_local int test_int = 0;
thread_local char test_char = 0;
// TDATA area
thread_local char test_array[3] = {1, 2, 3};
thread_local int64_t test_i64 = 0x11ABCDEF22ABCDEF;

void Service::start()
{
  int bss_local = 0;
  int data_local = 1;
  // TBSS area
  assert(test_int == 0);
  assert(test_char == 0);
  // modify TBSS
  test_int = 1;
  assert(test_int == 1);
  // TDATA area
  assert(test_array[0] == 1);
  assert(test_array[1] == 2);
  assert(test_array[2] == 3);
  assert(test_i64 == 0x11ABCDEF22ABCDEF);
  // modify TDATA area
  test_array[0] = 44;
  assert(test_array[0] == 44);
  assert(test_array[1] == 2);
  assert(test_array[2] == 3);
  // verify locals
  assert(bss_local == 0);
  assert(data_local == 1);
  printf("SUCCESS\n");
}
