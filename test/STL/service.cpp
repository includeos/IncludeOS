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
#include <stdio.h>
#include <vector>
#include <map>

//#include <thread> => <thread> is not supported on this single threaded system

void Service::start()
{
  
  // Wonder when these are used...?
  std::set_terminate([](){ printf("CUSTOM TERMINATE Handler \n"); });
  std::set_new_handler([](){ printf("CUSTOM NEW Handler \n"); });

  
  // TODO: find some implementation for long double, or not... or use double
  //auto sine = sinl(42);


  printf("TESTING STL Basics \n");
  std::cout << "[x] std::cout works and so does" << std::endl;
  std::cout << "[x] std::endl" << std::endl;
  
  std::vector<int> integers={1,2,3};
  std::map<const char*, int> map_of_ints={std::make_pair("First",42) , std::make_pair("Second",43)};
  for (auto i : integers)
    printf("Integer %i \n",i);
  
  printf("[%s] First from map: %i \n", map_of_ints["First"] == 42 ? "x" : " ", map_of_ints["First"]);
  printf("[%s] Second from map: %i \n", map_of_ints["Second"] == 43 ? "x" : " ", map_of_ints["Second"]);
  
 
  [] (void) { printf("[x] Lambda is called \n"); } ();
  
  std::string str = "[x] std::string to c-string conversion  works";
  printf("%s\n", str.c_str());
  
  
}
