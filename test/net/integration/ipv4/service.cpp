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

#include <os>

#include "ipv4_module_test.hpp"

void Service::start(const std::string&)
{
  const auto number_of_failed_tests = lest::run(ipv4_module_test, {"-p"});

  if (number_of_failed_tests) {
    printf("%d %s failed\n", number_of_failed_tests, (number_of_failed_tests == 1 ? "test has" : "tests have"));
    MYINFO("FAILURE");
  } else {
    printf("%s\n", "All tests passed");
    MYINFO("SUCCESS");
  }  
}
