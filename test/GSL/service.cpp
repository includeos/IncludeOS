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
#include <cassert>

#define GSL_THROW_ON_CONTRACT_VIOLATION
#include <gsl.h>
#include <lest.hpp>


int clock_gettime(clockid_t clk_id, struct timespec *tp){
  (void*)clk_id;
  (void*)tp;
  return 0;
};


#define MYINFO(X,...) INFO("Test GSL",X,##__VA_ARGS__)

static int divcount_ = 0;

const lest::test test_basic_gsl[] = {
  {

    SCENARIO ("Basic Expects / Ensures behave as expected") {

      GIVEN ("A simple division function, requiring positive integers") {

        class Math {
        public:
          static int div (int x, int y) {
            Expects( y > 0 );

            int prevcount_ = divcount_;

            auto result = x / y;
            divcount_++;

            Ensures(result == x / y);
            Ensures(divcount_ > 0);
            Ensures(divcount_ == prevcount_ + 1);

            return result;

          }
        };

        WHEN ("y is zero") {
          int y = 0;

          THEN ("Division by y throws") {
            EXPECT_THROWS( Math::div(100, y) );
          }

        }

        WHEN ("y is positive") {
          int y = 4;

          THEN ("Division by y doesn't throw") {
            EXPECT_NO_THROW( Math::div(100, y) );
          }

          AND_THEN("Division gives the correct result"){
            EXPECT( Math::div(100,y) == 100 / y );
          }
        }

        WHEN ("y is negative"){
          int y = -90;

          THEN ("Divsion by y throws") {}
          EXPECT_THROWS( Math::div(100, y) );

          AND_THEN("Division should have succeeded twice"){
            EXPECT(divcount_ == 2);
          }
        }
      }
    }
  },

  {
    SCENARIO("We can use span to replace pointer-and-size interfaces") {

      GIVEN ("A (member) function with a span parameter") {

        class Mem {
        public:
          static unsigned int count(gsl::span<char> chars){

            int i = 0;
            for ( auto c : chars ) {
              printf("%c ", c);
              i++;
            }

            printf("\n");
            return i;
          }


        };

        WHEN ("We pass a raw pointer") {
          char* name = (char*)"Bjarne Stroustrup";

          THEN("if we lie to the about our size, span can't save us from overflow / underflow") {
            EXPECT( Mem::count({name, 100}) != std::string(name).size());
            EXPECT( Mem::count({name, 10}) != std::string(name).size());
          }

          AND_THEN("If caller keeps track of the size, it will still work"){
            EXPECT( Mem::count({name, 17}) == std::string(name).size());
          }

        }

        WHEN ("We use std::array") {
          std::array<char,30> my_array {'G','S','L',' ','l','o', 'o', 'k', 's', ' ', 'n', 'i', 'c', 'e'};
          THEN("we're perfectly safe"){
            EXPECT( Mem::count(my_array) == my_array.size() );
          }
        }

        WHEN ("We use normal array") {
          char str2 [] = {49, 50, 51, 52, 53};

          THEN ("Span helps us avoid decay to pointer") {
            EXPECT( Mem::count(str2) == sizeof(str2));
          }
        }
      }

      GIVEN ("A (Bad) span-interface that doesn't do any bounds checking") {

        class Bad {
        public:
          static unsigned char eighth(gsl::span<char> chars){
            return chars[8];
          }
        };

        WHEN ("we pass in sufficient data") {
          char* character = (char*) "Bjarne \"Yoda\" Stroustrup leads the Jedi council with wisdom";
          THEN("you can access the elements of the span using the index operator"){
            EXPECT(Bad::eighth({character, 20}) == 'Y');
          }

        }

        WHEN ("we pass in too little data") {
          char* character = (char*) "Yoda";
          THEN("span saves us from complete embarrasment") {
            EXPECT_THROWS(Bad::eighth({character, 4}));
          }

        }

      }


    }
  }
};

void Service::start()
{

  // Lest takes command line params as vector
  auto failed = lest::run(test_basic_gsl, {"-p"});

  assert(not failed);

  MYINFO("SUCCESS");


}
