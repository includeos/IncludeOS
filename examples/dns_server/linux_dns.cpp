
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

int main()
{
  std::cout << "*** Service is up - with OS Included! ***" << std::endl;
  std::cout << "Starting DNS prototype\n";
    
  /// www.google.com ///
  std::vector<IP4::addr> mapping1;
  mapping1.push_back( { 213, 155, 151, 187 } );
  mapping1.push_back( { 213, 155, 151, 185 } );
  mapping1.push_back( { 213, 155, 151, 180 } );
  mapping1.push_back( { 213, 155, 151, 183 } );
  mapping1.push_back( { 213, 155, 151, 186 } );
  mapping1.push_back( { 213, 155, 151, 184 } );
  mapping1.push_back( { 213, 155, 151, 181 } );
  mapping1.push_back( { 213, 155, 151, 182 } );
  
  eastl::map<eastl::string, eastl::vector<IP4::addr>> lookup;
  lookup["www.google.com."] = mapping1;
  ///               ///
  
  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0)
  {
	perror("socket() failed");
	return -1;
  }
  
  
  in_addr lan_addr;
  inet_pton(AF_INET, "128.39.74.243", &lan_addr);
  
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
  int bytes = recvfrom(fd, buffer, BUFSIZE, 0, (sockaddr*) &remote, &addrlen);
  
  if (bytes > 0)
  {
	printf("Received %d boats.\n", bytes);
	
	int packetlen = DNS::createResponse(*(DNS::header*) buffer,
	
		[&lookup] (const std::string& name) ->
		std::vector<net::IP4::addr>*
		{
			auto it = lookup.find(name);
			if (it == lookup.end()) return nullptr;
			return &lookup[name];
		});
	
	int sent = sendto(fd, buffer, packetlen, 0,
               (sockaddr*) &remote, addrlen);
	
	printf("Wrote %d boats.\n", sent);
  }
  
  std::cout << "Service out!" << std::endl;
}
