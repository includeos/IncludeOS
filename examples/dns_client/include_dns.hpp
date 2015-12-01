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

#ifndef LINUX_DNS_HPP
#define LINUX_DNS_HPP

#include "dns_request.hpp"

#include <net/inet>
#include <net/util.hpp>
#include <net/class_packet.hpp>
#include <net/class_udp.hpp>
#include <list>
#include <string>
#include <memory>
#include <iostream>

#include <memstream>

extern net::Inet* network;

class PacketStore
{
public:
  std::shared_ptr<net::Packet> getPacket()
  {
    if (_queue.empty())
       panic("Packet store: out of packets");
    
    auto elt = *_queue.begin();
    _queue.pop_front();
    _queue.push_back(elt);
    return elt;
  }
  
  PacketStore(uint16_t n, uint32_t size)
    : _n(n), _bufsize(size), _pool(nullptr)
  {       
    this->_pool = new uint8_t[n * size];
    
    for(int i = 0; i < _n; i++)
    {
      _queue.push_back
        (std::make_shared<net::Packet>(&_pool[i * size], size, net::Packet::AVAILABLE));
    }
    
    std::cout << "<PacketStore> Allocated " << n*size << " byte buffer pool for packets" << std::endl;
  }
  
private:
	uint16_t _n;
	uint32_t _bufsize;
	uint8_t* _pool;
	std::list<std::shared_ptr<net::Packet> > _queue;
};
extern PacketStore packetStore;

class IncludeDNS : public AbstractRequest
{
public:
	IncludeDNS() : AbstractRequest() {}
	
	virtual void set_ns(unsigned nameserver)
	{
		using namespace net;
		
		network->udp_listen(DNS::DNS_SERVICE_PORT,
		[this] (net::Packet_ptr pckt)
		{
			std::cout << "*** Response from DNS server:" << std::endl;
			auto data_loc = pckt->buffer() + sizeof(UDP::full_header);
			auto data_len = pckt->len() - sizeof(UDP::full_header);
			
      // parse incoming data
			//this->buffer = (char*) data_loc;
      streamucpy(this->buffer, data_loc, data_len);
			
      //read();
      req.parseResponse(this->buffer);
      
      std::cout << "*** Printing results:" << std::endl;
			// print results
			print();
			
			return 0;
		});
		
		this->nameserver = nameserver;
	}
	
private:
	virtual bool send(const std::string& hostname, int messageSize)
	{
		using namespace net;
		
		// send request to nameserver
		std::cout << "Resolving " << hostname << "..." << std::endl;
		
		auto pckt = packetStore.getPacket();
		UDP::full_header& header = *(UDP::full_header*) pckt->buffer();
		
		// Populate outgoing UDP header
		header.udp_hdr.dport = htons(DNS::DNS_SERVICE_PORT);
		header.udp_hdr.sport = htons(DNS::DNS_SERVICE_PORT);
		header.udp_hdr.length = htons(sizeof(UDP::udp_header) + messageSize);
    
		// Populate outgoing IP header
		header.ip_hdr.saddr = network->ip4(ETH0);
		header.ip_hdr.daddr = IP4::addr { whole: this->nameserver };
		header.ip_hdr.protocol = IP4::IP4_UDP;
		
		// packet payload
		//streamucpy((char*) pckt->buffer() + sizeof(UDP::full_header), this->buffer, messageSize);
    memcpy(pckt->buffer() + sizeof(UDP::full_header), this->buffer, messageSize);
		pckt->set_len(sizeof(UDP::full_header) + messageSize);
    
		std::cout << "Sending DNS query..." << std::endl;
		network->udp_send(pckt);
		return true;
	}
	virtual bool read()
	{
    std::cout << "IM INSIDE READ" << std::endl;
		// parse response from nameserver
		//req.parseResponse(buffer);
		return true;
	}
	
	unsigned nameserver;
};

#endif
