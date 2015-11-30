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
