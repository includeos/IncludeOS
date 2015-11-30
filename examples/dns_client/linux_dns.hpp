// Part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and  Alfred Bratterud
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

#ifndef LINUX_DNS_HPP
#define LINUX_DNS_HPP

#include "dns_request.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SOCKET_ERROR  -1
typedef int socket_t;

class LinuxDNS : public AbstractRequest
{
public:
	LinuxDNS() : AbstractRequest() {}
	
	void set_ns(const std::string& nameserver)
	{
		sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		
		// destination nameserver
		dest.sin_family = AF_INET;
		dest.sin_port   = htons(DNS_PORT);
		dest.sin_addr.s_addr = inet_addr(nameserver.c_str());
	}
	
private:
	bool send(const std::string& hostname, int messageSize)
	{
		// send request to nameserver
		printf("Resolving %s...", hostname.c_str());
		int sent = sendto(sock, buffer, messageSize, 0, (struct sockaddr*) &dest, sizeof(dest));
		
		if (sent == SOCKET_ERROR)
		{
			printf("error %d: %s\n", errno, strerror(errno));
			return false;
		}
		printf("Done\n");
		return true;
	}
	bool read()
	{
		// read response
		socklen_t read_len = sizeof(dest);
		int readBytes = recvfrom(sock, buffer, 65536, 0, (struct sockaddr*) &dest, &read_len);
		
		printf("Receiving answer...");
		if (readBytes == SOCKET_ERROR)
		{
			printf("error %d: %s\n", errno, strerror(errno));
		}
		else if (readBytes == 0)
		{
			printf("closed prematurely\n");
			return false;
		}
		printf("Received.\n");
		return true;
	}
	
	sockaddr_in dest;
	socket_t sock;
};

#endif
