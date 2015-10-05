//#define DEBUG // Debug supression

#include <os>
#include <list>
#include <net/inet4>
#include <vector>

using namespace std;
using namespace net;

uint8_t* buf = 0;
int bufsize = 0;
uint8_t* prev_data = 0;

class PacketStore {
public:
  shared_ptr<Packet> getPacket(){
    if(_queue.empty())
      panic("Packet store is out of packets");
    auto elt = *_queue.begin();
    _queue.pop_front();
    _queue.push_back(elt);
    return elt;
  };
  
  
  PacketStore(uint16_t n, uint32_t size):
    _n(n), _bufsize(size)
  {       
    _pool = new uint8_t[n*size];
    for(int i = 0; i < _n; i++){
      _queue.push_back
        (make_shared<Packet>(&_pool[i*_bufsize],_bufsize,Packet::AVAILABLE));
    }
        
    printf("<PacketStore> Allocated %ul byte buffer pool for packets \n",n*size);
  };
  
private:
  uint16_t _n = 100;
  uint32_t _bufsize = 1500;
  uint8_t* _pool = 0;
  std::list<shared_ptr<Packet> > _queue;
}UDP_store(100,1500);


void Service::start()
{
  
  // Assign IP and netmask
  Inet4::ifconfig(net::ETH0, {{ 10, 0, 0, 10 }}, {{ 255, 255, 0, 0 }});
  
  Inet4* net(Inet4::up());
  

  cout << "...Starting UDP server on IP " 
       << net->ip4(net::ETH0).str()
       << endl;

  //A one-way UDP server (a primitive test)
  net->udp_listen(8080,[net](std::shared_ptr<net::Packet> pckt){
      
      UDP::full_header* full_hdr = (UDP::full_header*)pckt->buffer();
      UDP::udp_header* hdr = &full_hdr->udp_hdr;
      
      int data_len = __builtin_bswap16(hdr->length) - sizeof(UDP::udp_header);
      auto data_loc = pckt->buffer() + sizeof(UDP::full_header);
      
      debug("<APP SERVER> Got %i b of data from %s:%i -> %s:%i\n",
            data_len, full_hdr->ip_hdr.saddr.str().c_str(), 
            __builtin_bswap16(hdr->sport),
            full_hdr->ip_hdr.daddr.str().c_str(), 
            __builtin_bswap16(hdr->dport));
      
      
      // Craft response      
      string response(string((const char*)data_loc,data_len));
      
      auto pckt_out = UDP_store.getPacket();
      auto buf = pckt_out->buffer();
      
       strcpy((char*)buf + sizeof(UDP::full_header),response.c_str());
       buf[data_len]=0;
       buf[data_len + 1]=0;
       
       debug("Reply: '%s' \n",buf+sizeof(UDP::full_header));

              
       /** Populate outgoing UDP header 
	   @todo: This should be done in sockets, obviously
	*/
       UDP::full_header* full_hdr_out = (UDP::full_header*)buf;
       full_hdr_out->udp_hdr.dport = hdr->sport;
       full_hdr_out->udp_hdr.sport = hdr->dport;
       
       pckt_out->set_len(response.size() + sizeof(UDP::full_header));
       
       debug("<APP SERVER> Sending %li b - udp-size: %i \n",
             response.size(), __builtin_bswap16(full_hdr_out->udp_hdr.length));
       
       
       /** Populate outgoing IP header */
       full_hdr_out->ip_hdr.saddr = full_hdr->ip_hdr.daddr;
       full_hdr_out->ip_hdr.daddr = full_hdr->ip_hdr.saddr;
       full_hdr_out->ip_hdr.protocol = IP4::IP4_UDP;
              
       net->udp_send(pckt_out);
       
       return 0;
    });
  
  cout << "<APP SERVER> Listening to UDP port 8080 " << endl;
  
}
