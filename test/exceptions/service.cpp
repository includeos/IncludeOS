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

class CustomException : public std::runtime_error {
  using runtime_error::runtime_error;
};

void Service::start()
{
  
  printf("TESTING Exceptions \n");
  
  const char* error_msg = "Crazy Error!";
  
  try {
    printf("[x] Inside try-block \n");
    if (OS::uptime() > 0.1){
      std::runtime_error myexception(error_msg);
      throw myexception;
    }

  }catch(std::runtime_error e){
    
    printf("[%s] Caught runtime error: %s \n", std::string(e.what()) == std::string(error_msg) ? "x" : " " ,e.what());
    
  }catch(...) {
    
    printf("[ ] Caught something - but not what we expected \n");
    
  }
  
  std::string custom_msg = "Custom exceptions are useful";
  std::string cought_msg = "";
  
  try {
    // Trying to throw a custom exception
    throw CustomException(custom_msg);
  } catch (CustomException e){
    
    cought_msg = e.what();
    
  } catch (...) {
    
    printf("[ ] Couldn't catch custom exception \n");
    
  }
  
  assert(cought_msg == custom_msg);
  printf("[x] Cought custom exception \n");
}
