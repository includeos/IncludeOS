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

/**
   A very superficial test to verify that basic STL is working
   This is useful when we mess with / replace STL implementations

 **/


#include <os>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <map>
#include <functional>
#include <cstring>

#define MYINFO(X,...) INFO("Test STL",X,##__VA_ARGS__)

void Service::start()
{

  // Wonder when these are used...?
  std::set_terminate([](){ printf("CUSTOM TERMINATE Handler \n"); });
  std::set_new_handler([](){ printf("CUSTOM NEW Handler \n"); });


  // TODO: find some implementation for long double, or not... or use double
  //auto sine = sinl(42);


  printf("*** Testing STL Basics - must be verified from the outside ***\n");
  MYINFO("The following two lines should be identical, including newline");

  printf("\tprintf and std::endl yields the same output\n");
  std::cout << "\tprintf and std::endl yields the same output\n";

  MYINFO("Expect Integer 1-3 to be printed");
  std::vector<int> integers={1,2,3};
  std::map<const char*, int> map_of_ints={std::make_pair("First",42) , std::make_pair("Second",43)};
  for (auto i : integers)
    CHECK(i,"Integer %i",i);

  MYINFO("Check basic map functionality")
  CHECK(map_of_ints["First"] == 42, "First from map: %i",  map_of_ints["First"]);
  CHECK(map_of_ints["Second"] == 43,"Second from map: %i", map_of_ints["Second"]);

  MYINFO("Call a std::function lambda immediately");
  std::function<void()> my_lambda = [] (void) { CHECK(1," Lambda is called"); };
  my_lambda();

  INFO("Test STL", "String to c-string conversion");

  const char* orig = "std::string to c-string conversion works";
  std::string str = "std::string to c-string conversion works";

  CHECK(strcmp(str.c_str(),orig) == 0, "std::string to c-string conversion works");

}
