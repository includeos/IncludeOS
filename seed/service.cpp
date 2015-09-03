#include <os>
#include <vector>
#include <exception>
#include <stdio.h>

#include <iostream>

#undef stdout
extern __FILE* stdout;

/*
template class
std::basic_ostream<char, std::char_traits<char> >&
std::basic_ostream<char, std::char_traits<char> >::write(const char* __s, std::streamsize __n);
*/

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
  
  // try newline
  std::cout << "testing std::cout\n";
  validate();
  
  // try flushing
  std::cout.flush();
  validate();
  
  // try manually appending newline char
  std::cout.put('\n');
  validate();
  
  // writing newline string to stream
  std::cout.write("\r\n", 2l);
  validate();
  
  // flushing indirectly
  std::cout << std::flush;
  validate();
  
  // .. and the one that crashes for some unknown reason
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
