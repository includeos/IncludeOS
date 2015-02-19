#ifndef LINUX_DNS_HPP
#define LINUX_DNS_HPP

#include "dns_request.hpp"

#include <net/inet>
#include <net/class_packet.hpp>
#include <net/class_udp.hpp>
#include <list>
#include <string>
#include <memory>
#include <iostream>

extern net::Inet* network;
extern unsigned short ntohs(unsigned short sh);
#define htons ntohs

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
	
	void set_ns(unsigned nameserver)
	{
		using namespace net;
		
		network->udp_listen(DNS_PORT,
		[this] (std::shared_ptr<net::Packet>& pckt)
		{
			std::cout << "*** Response from DNS server:" << std::endl;
			auto data_loc = pckt->buffer() + sizeof(UDP::full_header);
			
			// parse incoming data
			this->buffer = (char*) data_loc;
			read();
			// print results
			print();
			
			return 0;
		});
		
		this->nameserver = nameserver;
	}
	
private:
	bool send(const std::string& hostname, int messageSize)
	{
		using namespace net;
		
		// send request to nameserver
		std::cout << "Resolving " << hostname << "..." << std::endl;
		
		auto pckt = packetStore.getPacket();
		UDP::full_header& header = *(UDP::full_header*) pckt->buffer();
		
		// Populate outgoing UDP header
		header.udp_hdr.dport = htons(DNS_PORT);
		header.udp_hdr.sport = htons(DNS_PORT);
		header.udp_hdr.length = htons(sizeof(UDP::udp_header) + messageSize);
    
		// Populate outgoing IP header
		header.ip_hdr.saddr = network->ip4(ETH0);
		header.ip_hdr.daddr = IP4::addr { whole: this->nameserver };
		header.ip_hdr.protocol = IP4::IP4_UDP;
		
		// packet payload
		memcpy(pckt->buffer() + sizeof(UDP::full_header), this->buffer, messageSize);
		
		std::cout << "Sending DNS query..." << std::endl;
		network->udp_send(pckt);
		return true;
	}
	bool read()
	{
		// parse response from nameserver
		req.parseResponse(buffer);
		
		print();
		return true;
	}
	
	unsigned nameserver;
};

#endif
