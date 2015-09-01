#include <os>
#include <vector>
#include <exception>
#include <stdio.h>

#include <iostream>

#undef stdout
extern __FILE* stdout;

void validate()
{
  if ( (std::cout.rdstate() & std::ios::failbit ) != 0 )
    printf("std::cout failbit was set\n");
  if ( (std::cout.rdstate() & std::ios::badbit ) != 0 )
    printf("std::cout badbit was set\n");
}

void Service::start()
{
  printf("fflush test: ");
  fflush(stdout);
  
  std::cout << "testing std::cout\n";
  validate();
  
  // try flushing
  std::cout.flush();
  validate();
  
  // try ending line
  std::cout.put('\n');
  validate();
  
  // try writing newline
  std::cout.write("\r\n", (int) 2);
  validate();
  
  // and...
  std::cout << std::endl;
  validate();
  return;
  
  std::vector<int> integers={1,2,3};
  std::map<const char*, int> map_of_ints={std::make_pair("First",42) , std::make_pair("Second",43)};
  
  [] (void) { printf("Hello lambda\n"); } ();
  
  for (auto i : integers)
    printf("Integer %i \n",i);
  
  printf("First from map: %i \n", map_of_ints["First"]);
  printf("Second from map: %i \n", map_of_ints["Second"]);
  
  std::string str = "Hello std::string";
  printf("%s\n", str.c_str());
  
  printf("before exception\n");
  /*
  try
  {
    throw std::string("hei");
  }
  catch (...)
  {
    printf("Caught exception: %s \n", str.c_str());
  }*/
  throw "hei";
  printf("after exception\n");
}
