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
#include <cassert>

class global {
  static int i;
public:
  global(){
    CHECK(1,"Global constructor printing %i",++i);
  }

  void test(){
    CHECK(1,"C++ constructor finds %i instances",i);
  }

  int instances(){ return i; }

  ~global(){
    CHECK(1,"C++ destructor deleted 1 instance,  %i remains",--i);
  }

};


int global::i = 0;

global glob1;

int _test_glob2 = 1;
int _test_glob3 = 1;

__attribute__ ((constructor)) void foo(void)
{
  _test_glob3 = 0xfa7ca7;
}



void Service::start(const std::string&)
{
  INFO("Test CRT","Testing C runtime \n");

  CHECKSERT(_test_glob3 == 0xfa7ca7, "Global C constructors in service");
  CHECKSERT(_test_glob2 == 1, "Global int initialization in service");

  global* glob2 = new global();;
  glob1.test();
  CHECKSERT(glob1.instances() == 2, "Local C++ constructors in service");

  delete glob2;
  CHECKSERT(glob1.instances() == 1, "C++ destructors in service");


  INFO("Test CRT", "SUCCESS");
}
