// Part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and  Alfred Bratterud
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

class global {
  static int i;
public:
  global(){
    printf("[*] Global constructor printing %i \n",++i);
  }
  
  void test(){
    printf("[*] C++ constructor finds %i instances \n",i);
  }
  
  int instances(){ return i; }
  
  ~global(){
    printf("[*] C++ destructor deleted 1 instance,  %i remains \n",--i);
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



void Service::start()
{
  
  printf("TESTING C runtime \n");

  printf("[%s] Global C constructors in service \n", 
         _test_glob3 == 0xfa7ca7 ? "x" : " ");
  
  printf("[%s] Global int initialization in service \n", 
         _test_glob2 == 1 ? "x" : " ");
  
  
  global* glob2 = new global();;
  glob1.test();
  printf("[%s] Local C++ constructors in service \n", glob1.instances() == 2 ? "x" : " ");

  
  delete glob2;
  printf("[%s] C++ destructors in service \n", glob1.instances() == 1 ? "x" : " ");
  
  
  
  
}
