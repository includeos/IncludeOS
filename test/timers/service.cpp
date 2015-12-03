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
#include <iostream>

using namespace std::chrono;

void Service::start()
{
  
  std::vector<int> integers={1,2,3};
  
  printf("TESTING Timers \n");
  auto& time = PIT::instance();
  
  
  int x = 42;
  
  // Write something in a while
  time.onTimeout(7s, [x](){ printf("7 seconds passed...%i \n",x); });
  
  // Write a dot in a second
  time.onTimeout(1s, [](){ printf("One second passed...\n"); });
  
  // Write something in half a second
  time.onTimeout(500ms, [](){ printf("Half a second passed...\n"); });
  

  time.onTimeout(3s, [](){ printf("Three second passed...\n"); });

   
  // You can also use the std::function interface (which is great)
  std::function<void()> in_a_second = [integers](){  
    std::cout << "Hey - this is a std::function - it knows about integers:" << std::endl;
    for (auto i : integers)
      std::cout << i << std::endl;    
  };
  
  time.onTimeout(1s, in_a_second);
 
  time.onRepeatedTimeout(1s, []{ printf("1sec. PULSE \n"); });
  time.onRepeatedTimeout(2s, []{ printf("2sec. PULSE, "); }, 
			 
			 // A continue-condition. The timer stops when false is returned
			 []{ 
			   static int i = 0; i++; 
			   printf("%i / 10 times \n", i);
			   if (i >= 10) {
			     printf("2sec. pulse DONE!");
			     return false; 
			   }
			   return true;  });

  
}
