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
