// Part of the IncludeOS Unikernel  - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and  Alfred Bratterud. 
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

//DNS_server myDnsServer;

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace net;

int main(int argc, char** argv)
{
  
  if(argc < 2){
    std::cout << "USAGE: " << (const char*)argv[0] << " <interface IP>" << std::endl;
    return 0;
  }
  
  std::cout << "*** Service is up - with OS Included! ***" << std::endl;
  std::cout << "Starting DNS prototype\n";
    
  DNS_server::init();
  
  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0)
  {
	perror("socket() failed");
	return -1;
  }
  
  
  in_addr lan_addr;
  inet_pton(AF_INET, "10.0.0.1", &lan_addr);
  
  struct sockaddr_in myaddr;
  myaddr.sin_family = AF_INET;
  myaddr.sin_addr.s_addr = lan_addr.s_addr;
  myaddr.sin_port = htons(DNS::DNS_SERVICE_PORT);
  
  if (bind(fd, (struct sockaddr *) &myaddr, sizeof(myaddr)) < 0)
  {
    perror("bind failed");
    return -1;
  }
  std::cout << "<DNS SERVER> Listening on UDP port 53" << std::endl;
  
  static const int BUFSIZE = 1500;
  char* buffer = new char[BUFSIZE];
  
  struct sockaddr_in remote;
  socklen_t addrlen = sizeof(remote);
  
  //int recvfrom(int socket, void *restrict buffer, size_t length, int flags, struct sockaddr *restrict src_addr, socklen_t *restrict *src_len)
  
  while(1){
    int bytes = recvfrom(fd, buffer, BUFSIZE, 0, (sockaddr*) &remote, &addrlen);
    
    if (bytes > 0)
      {
        printf("Received %d boats.\n", bytes);
	
	int packetlen = 
          DNS::createResponse(*(DNS::header*) buffer,
                              [] (const std::string& name) ->
                              std::vector<net::IP4::addr>*
                              {
                                return DNS_server::lookup(name);
                              });
	
	int sent = sendto(fd, buffer, packetlen, 0,
                          (sockaddr*) &remote, addrlen);
	
	printf("Wrote %d boats.\n", sent);
      }
    
  
  }
  
  std::cout << "Service out!" << std::endl;
}
  
  
  
