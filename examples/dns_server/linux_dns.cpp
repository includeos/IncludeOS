
// Locals
#include "dns_server.hpp"
#include <stdlib.h>

// EASTL new/delete
//void* operator new[](size_t size, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
void* operator new[](size_t size, const char*, int, unsigned, const char*, int)
{
  //printf("[new:%lu] %s %d %d from %s:%d\n", size, pName, flags, debugFlags, file, line);
  return malloc(size);
}
//void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
void* operator new[](size_t size, size_t, size_t, const char*, int, unsigned, const char*, int)
{
  //printf("[new:%lu] %s %d %d from %s:%d\n", size, pName, flags, debugFlags, file, line);
  return malloc(size);
}

void* operator new (size_t size)
{
  return malloc(size);
}
void* operator new[] (size_t size)
{
  return malloc(size);
}


// placement new/delete
void* operator new  (size_t, void* p)  { return p; }
void* operator new[](size_t, void* p)  { return p; }

extern "C" void __assert_func(int){};

DNS_server myDnsServer;

int main(){
  std::cout << "*** Service is up - with OS Included! ***" << std::endl;
  std::cout << "Starting DNS prototype\n";
    
  /// www.google.com ///
  std::vector<net::IP4::addr> mapping1;
  mapping1.push_back( { 213, 155, 151, 187 } );
  mapping1.push_back( { 213, 155, 151, 185 } );
  mapping1.push_back( { 213, 155, 151, 180 } );
  mapping1.push_back( { 213, 155, 151, 183 } );
  mapping1.push_back( { 213, 155, 151, 186 } );
  mapping1.push_back( { 213, 155, 151, 184 } );
  mapping1.push_back( { 213, 155, 151, 181 } );
  mapping1.push_back( { 213, 155, 151, 182 } );
  
  myDnsServer.addMapping("www.google.com.", mapping1);
  ///               ///
  
  //class Inet{} inet*;
  net::Inet* inet;
  myDnsServer.start(inet);
  std::cout << "<DNS SERVER> Listening on UDP port 53" << std::endl;
  
  std::cout << "Service out!" << std::endl;
}
