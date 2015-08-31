#include <os>
#include <vector>
#include <exception>
#include <stdio.h>

template class std::basic_string <char>;

#include <iostream>
void test_iostream()
{
  //std::cout << "std::cout TEST" << std::endl;
}

void Service::start()
{
  std::string str = "Hello strings";
  printf("Hello world\n");
  
  std::vector<int> integers={1,2,3};
  std::map<const char*, int> map_of_ints={std::make_pair("First",42) , std::make_pair("Second",43)};
  
  [] (void) { printf("Hello lambda\n"); } ();
  
  for (auto i : integers)
    printf("Integer %i \n",i);
  
  printf("First from map: %i \n", map_of_ints["First"]);
  printf("Second from map: %i \n", map_of_ints["Second"]);
  
  try
  {
    throw std::string("hei");
  }
  catch (...)
  {
    printf("Caught exception: %s \n", str.c_str());
  }
  
  test_iostream();
}
