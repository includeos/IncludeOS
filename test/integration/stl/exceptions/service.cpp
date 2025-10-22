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
#include <cassert>
#include <lest/lest.hpp>

#define MYINFO(X,...) INFO("Test exceptions",X,##__VA_ARGS__)

const lest::test tests[] = {
  {
    SCENARIO("exceptions can be thrown and caught") {
      GIVEN ("a custom exception class") {
        class CustomException : public std::runtime_error {
          using runtime_error::runtime_error;
        };

        THEN("a runtime exception should be caught") {
          const char *error_msg = "Crazy Error!";

          bool caught = false;
          try {
            // We want to always throw, but avoid the compiler optimizing
            // away the test. Also nice to have this test talk to the OS a little.
            if (rand()){
              std::runtime_error myexception(error_msg);
              throw myexception;
            }
          } catch(const std::runtime_error& e) {
            caught = std::string(e.what()) == std::string(error_msg);
          }
          EXPECT(caught);
        }

        AND_THEN("a custom exception should also be caught") {
          std::string custom_msg = "Custom exceptions are useful";
          std::string caught_msg = "";

          try {
            throw CustomException(custom_msg);
          } catch (const CustomException& e){
            caught_msg = e.what();
          }

          EXPECT(caught_msg == custom_msg);
        };
      }
    }
  },
};

void Service::start(const std::string&)
{
  MYINFO ("Running LEST-tests");
  auto failed = lest::run(tests, {"-p"});
  assert(not failed);

  MYINFO("Part 1 OK. Now throwing whithout try-catch which should panic");
  throw std::runtime_error("Uncaught exception expecting panic");
}
