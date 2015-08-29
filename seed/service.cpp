#include <os>
#include <iostream>
#include <vector>
#include <exception>

//using namespace std;

void Service::start()
{
  
  std::vector<int> integers={1,2,3};
  std::map<const char*, int> map_of_ints={std::make_pair("First",42) , std::make_pair("Second",43)};
  
  //std::string str = "Hello strings";
  
  printf("Hello world\n");
  
  [] (void) { printf("Hello lambda\n"); } ();
  
  for (auto i : integers)
    printf("Integer %i \n",i);
  
  printf("First from map: %i \n", map_of_ints["First"]);
  printf("Second from map: %i \n", map_of_ints["Second"]);
  try {
    throw "hei";
  }catch(const char* str){
    printf("Caught exception: %s \n",str);
  }
  
}
