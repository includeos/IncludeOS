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
#include <lest.hpp>

extern int f1_data;
extern int f2_data;
extern int f3_data;
extern int my_init_functions;

const lest::test specification[] =
  {
    {
      CASE( "Make sure the custom initialization functions were called" )
      {
        EXPECT(f1_data == 0xf1);
        EXPECT(f2_data == 0xf2);
        EXPECT(f3_data == 0xf417);
        EXPECT(my_init_functions == 3);
      }
    }
  };


void Service::start(const std::string&)
{
  INFO("Custom init test", "Testing the custom initialization");

  auto failed = lest::run(specification, {"-p"});
  Expects(not failed);

  INFO("Custom init test", "SUCCESS");

}
