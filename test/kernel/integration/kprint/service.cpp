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

#include <os>
#include "../../../../src/include/kprint"


void Service::start(const std::string&)
{
  INFO("service", "Testing kprint");

  kprintf("Test 2 I can print hex: 0x%x \n", 100);

  const char* format = "truncate %s \n";
  const char* str = "bla bla bla bla bla bla bla this part should be truncated END";
  kprintf("Test 3 Format string size: %i\n", strlen(format));

  // Expect this to print a string 2x the size of format
  kprintf(format, str);

  // Use the simple char* kprint function to indicate success
  // (newline should be added here since it's truncated in the previous test)
  kprint("\nSUCCESS");

}
