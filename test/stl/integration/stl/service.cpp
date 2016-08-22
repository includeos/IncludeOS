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
#include <cassert>
#include <lest.hpp>

using namespace std;

const lest::test specification[] =
  {
    {
      SCENARIO( "vectors can be sized and resized" "[vector]" )
      {

        GIVEN( "A vector with some items" ) {
          std::vector<int> v( 5 );

          EXPECT( v.size() == 5u );
          EXPECT( v.capacity() >= 5u );

          WHEN( "the size is increased" ) {
            v.resize( 10 );

            THEN( "the size and capacity change" ) {
              EXPECT( v.size() == 10u);
              EXPECT( v.capacity() >= 10u );
            }
          }
          WHEN( "the size is reduced" ) {
            v.resize( 0 );

            THEN( "the size changes but not capacity" ) {
              EXPECT( v.size() == 0u );
              EXPECT( v.capacity() >= 5u );
            }
          }
          WHEN( "more capacity is reserved" ) {
            v.reserve( 10 );

            THEN( "the capacity changes but not the size" ) {
              EXPECT( v.size() == 5u );
              EXPECT( v.capacity() >= 10u );
            }
            WHEN( "less capacity is reserved again" ) {
              v.reserve( 7 );

              THEN( "capacity remains unchanged" ) {
                EXPECT( v.capacity() >= 10u );
              }
            }
          }
          WHEN( "less capacity is reserved" ) {
            v.reserve( 0 );

            THEN( "neither size nor capacity are changed" ) {
              EXPECT( v.size() == 5u );
              EXPECT( v.capacity() >= 5u );
            }
          }
        }
      }
    }
  };

#define MYINFO(X,...) INFO("Test STL",X,##__VA_ARGS__)

void Service::start(const std::string&)
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
    CHECKSERT(i,"Integer %i",i);

  MYINFO("Check basic map functionality");
  CHECKSERT(map_of_ints["First"] == 42, "First from map: %i",  map_of_ints["First"]);
  CHECKSERT(map_of_ints["Second"] == 43,"Second from map: %i", map_of_ints["Second"]);

  MYINFO("Call a std::function lambda immediately");
  std::function<void()> my_lambda = [] (void) { CHECKSERT(1," Lambda is called"); };
  my_lambda();

  INFO("Test STL", "String to c-string conversion");

  const char* orig = "std::string to c-string conversion works";
  std::string str = "std::string to c-string conversion works";

  CHECKSERT(strcmp(str.c_str(),orig) == 0, "std::string to c-string conversion works");

  char* argv[2] {(char*)"test",(char*)"-p"};

  lest::run(specification, 2, argv);

  MYINFO("Expect panic after exit handler");
  at_quick_exit([] {
    CHECK(1,"Custom quick exit-handler called");
    MYINFO("SUCCESS");
  });

  quick_exit(0);

  CHECKSERT(false, "Quick exit never happend");
}
