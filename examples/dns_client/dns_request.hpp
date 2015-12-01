// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
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
