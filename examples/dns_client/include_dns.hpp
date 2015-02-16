#ifndef LINUX_DNS_HPP
#define LINUX_DNS_HPP

#include "dns_request.hpp"

#include <net/inet>
#include <net/class_packet.hpp>
#include <net/class_udp.hpp>
#include <list>
#include <string>
#include <memory>

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
  
  PacketStore(uint16_t n, uint32_t size):
    _n(n), _bufsize(size), _pool(nullptr)
  {       
    this->_pool = new uint8_t[n * size];
    
    for(int i = 0; i < _n; i++)
    {
      _queue.push_back
        (std::make_shared<net::Packet>(&_pool[i * size], size, net::Packet::AVAILABLE));
    }
        
    printf("<PacketStore> Allocated %li byte buffer pool for packets \n",n*size);
  }

  //PacketStore(int n, int size, caddr_t pool);
  
private:
	uint16_t _n;
	uint32_t _bufsize;
	uint8_t* _pool;
	std::list<std::shared_ptr<net::Packet> > _queue;
};

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
		printf("Resolving %s...", hostname.c_str());
		
		int len = sizeof(UDP::full_header) + messageSize;
		unsigned char* buf = new unsigned char[len]();
		
		//memset(buf, 0, sizeof(UDP::full_header));
		memcpy(buf + sizeof(UDP::full_header), this->buffer, len);
		
		std::shared_ptr<Packet> pckt(
			new Packet((uint8_t*) this->buffer, messageSize, Packet::DOWNSTREAM));
		
		// Populate outgoing UDP header
		UDP::full_header* full_hdr_out = (UDP::full_header*) pckt->buffer();
		full_hdr_out->udp_hdr.dport = DNS_PORT;
		full_hdr_out->udp_hdr.sport = DNS_PORT;
		full_hdr_out->udp_hdr.length = __builtin_bswap16(messageSize);
		
		// Populate outgoing IP header
		full_hdr_out->ip_hdr.saddr = IP4::addr { whole: this->nameserver };
		full_hdr_out->ip_hdr.daddr = network->ip4(ETH0);
		full_hdr_out->ip_hdr.protocol = IP4::IP4_UDP;
		
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
