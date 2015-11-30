#ifndef DNS_REQUEST_HPP
#define DNS_REQUEST_HPP

#include <net/dns/dns.hpp>

class AbstractRequest
{
public:
	AbstractRequest()
	{
		this->buffer = new char[65536];
	}
	~AbstractRequest()
	{
		delete[] this->buffer;
	}
	
	// create/open connection to remote part
	virtual void set_ns(unsigned nameserver) = 0;
	
	// send request to nameserver
	bool request(const std::string& hostname)
	{
		// create request to nameserver
		int messageSize = req.create(buffer, hostname);
		
		// send request
		return send(hostname, messageSize);
	}
	void print()
	{
		// print all the (currently) stored information
		req.print(buffer);
	}
	
protected:
	virtual bool send(const std::string& hostname, int messageSize) = 0;
	virtual bool read() = 0;
	
	net::DNS::Request req;
	char* buffer;
};

#endif
