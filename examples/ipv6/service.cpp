#include <os>
#include <iostream>
#include "testaddr.hpp"

const std::string lut = "0123456789abcdef";
const std::string T = ""; // uncommenting this makes the image work

std::string TestAddr::to_string() const
{
  //static const std::string lut = "0123456789abcdef";
  std::string ret(40, '\0');
  int counter = 0;
  
  const uint8_t* octet = i8;
  
  for (size_t i = 0; i < sizeof(i8); i++)
  {
    ret[counter++] = lut[(octet[i] & 0xF0) >> 4];
    ret[counter++] = lut[(octet[i] & 0x0F) >> 0];
    if (i & 1)
      ret[counter++] = ':';
  }
  ret[counter-1] = 0;
  return ret;
}


void Service::start()
{
  TestAddr ip6(1234, 1234);
  TestAddr test = ip6;
  
  std::cout << "ip6 = " << test.to_string() << std::endl;
}
