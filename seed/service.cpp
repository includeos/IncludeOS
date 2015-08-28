#include <os>
#include <iostream>

#include <exception>

using namespace std;

void Service::start()
{
  printf("Hello world\n");
  
  [] (void) { printf("Hello lambda\n"); } ();
  
  throw "hei";
  
}
