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
#include <cstdio>
#include <isotime>
#include <stdexcept>

void failing_function()
{
  throw std::invalid_argument( "received negative value" );
}

void Service::start(const std::string& args)
{
#ifdef __GNUG__
  printf("Built by g++ " __VERSION__ "\n");
#endif
  printf("Issue SVC\n");
  //SVC is not legal in EL1.. HVC and SMC is
  asm volatile ("svc #0"); //this will trigger a syn exception and return

  printf("SVC success\n");

  //wfi
//  while(1);
  // TODO fix this
/*  try {
    //raise std::exception("Test");
    throw 20;
    //failing_function();
  }
  catch(int e)
  {
    printf("Exception caught\n");
  }*/
  //TODO fix time handling
  //printf("Hello world! Time is now %s\n", isotime::now().c_str());
  printf("Args = %s\n", args.c_str());
  printf("Try giving the service less memory, eg. 3MB in vm.json\n");
}
#endif

const char* service_name__ = SERVICE_NAME;
const char* service_binary_name__ = SERVICE;
