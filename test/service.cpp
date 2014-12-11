//#define NDEBUG // Debug supression

#include <os>
#include <EASTL/list.h>
#include <net/inet>
#include <vector>

using namespace std;
using namespace net;

uint8_t* buf = 0;
int bufsize = 0;
uint8_t* prev_data = 0;

class global {
  static int i;
public:
  global(){
    printf("[*] Global constructor printing %i \n",++i);
  }
  
  void test(){
    printf("[*] C++ constructor finds %i instances \n",i);
  }
  
  int instances(){ return i; }
  
  ~global(){
    printf("[*] C++ destructor deleted 1 instance,  %i remains \n",--i);
  }
  
};

int global::i = 0;

global glob1;

int _test_glob2 = 1;
int _test_glob3 = 1;

__attribute__ ((constructor)) void foo(void)
{
  _test_glob3 = 0xfa7ca7;
}

/* @todo - make configuration happen here
void Service::init(){

  
  Inet net;
  auto eth0 = Dev::eth(0);
  auto mac = eth0.mac();
  
  net.ifconfig(ETH0,{192,168,mac.part[4],mac.part[5]},{255,255,0,0});
  //net.ifconfig(ETH0,{192,168,0,10}); => netmask == 255.255.255.0.
  //net.route("*",{192.168.0.1});
  
  
  Dev::eth(ETH0)
        
  //Dev::eth(1).dhclient();
  
  }*/


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
        (make_shared<Packet>(Packet(&_pool[i*size],size,Packet::AVAILABLE)));
    }
        
    printf("<PacketStore> Allocated %li byte buffer pool for packets \n",n*size);
  };

  //PacketStore(int n, int size, caddr_t pool);
  
private:
  uint16_t _n = 100;
  uint32_t _bufsize = 1500;
  uint8_t* _pool = 0;
  eastl::list<shared_ptr<Packet> > _queue;
}UDP_store(100,1500);


void Service::start()
{
  
  cout << "*** Service is up - with OS Included! ***" << endl;    
  
  printf("[%s] Global C constructors in service \n", 
         _test_glob3 == 0xfa7ca7 ? "x" : " ");
  
  printf("[%s] Global int initialization in service \n", 
         _test_glob2 == 1 ? "x" : " ");
  
  
  global* glob2 = new global();;
  glob1.test();
  printf("[%s] Local C++ constructors in service \n", glob1.instances() == 2 ? "x" : " ");

  
  delete glob2;
  printf("[%s] C++ destructors in service \n", glob1.instances() == 1 ? "x" : " ");
  
  

  auto& mac = Dev::eth(0).mac();
  Inet::ifconfig(net::ETH0,{mac.part[2],mac.part[3],mac.part[4],mac.part[5]},{255,255,0,0});
  
  /** Trying to access non-existing nic will cause a panic */
  //auto& mac1 = Dev::eth(1).mac();
  //Inet::ifconfig(net::ETH1,{192,168,mac1.part[4],mac1.part[5]},{255,255,0,0});
  
  //Inet* net 
  shared_ptr<Inet> net(Inet::up());
  

  cout << "...Starting UDP server on IP " 
       << net->ip4(net::ETH0).str()
       << endl;

    
  //A one-way UDP server (a primitive test)
  net->udp_listen(8080,[net](std::shared_ptr<net::Packet> pckt){
      
      UDP::full_header* full_hdr = (UDP::full_header*)pckt->buffer();
      UDP::udp_header* hdr = &full_hdr->udp_hdr;

      int data_len = __builtin_bswap16(hdr->length) - sizeof(UDP::udp_header);
      auto data_loc = pckt->buffer() + sizeof(UDP::full_header);
      
      // Netcat doesn't necessariliy zero-pad the string in UDP
      // ... But this buffer is const
      // auto data_end = data + hdr->length - sizeof(UDP::udp_header);
      // *data_end = 0; 
      
      debug("<APP SERVER> Got %i b of data (%i b frame) from %s:%i -> %s:%i\n",
            data_len, len, full_hdr->ip_hdr.saddr.str().c_str(), 
            __builtin_bswap16(hdr->sport),
            full_hdr->ip_hdr.daddr.str().c_str(), 
            __builtin_bswap16(hdr->dport));
      
      
      //printf("buf@0x%lx ",(uint32_t)buf);
      //printf("UDP from %s ", full_hdr->ip_hdr.saddr.str().c_str());
      
      /*
      printf("Got '");
      // Print the input
      for (int i = 0; i < data_len; i++)
        printf("%c", data_loc[i]);
      
      printf("' UDP data  from %s. (str: %s) \n", 
             full_hdr->ip_hdr.saddr.str().c_str(),data_loc);
      */
      // Craft response
      
       string response(string((const char*)data_loc,data_len));
       
       /*
       bufsize = response.size() + sizeof(UDP::full_header);
      
       // Ethernet padding if necessary
       if (bufsize < Ethernet::minimum_payload)
         bufsize = Ethernet::minimum_payload;
       
      
       if(buf)
         delete[] buf;
      
       buf = new uint8_t[bufsize]; 

       */
       
       auto pckt_out = UDP_store.getPacket();
       auto buf = pckt_out->buffer();
       
       strncpy((char*)buf + sizeof(UDP::full_header),response.c_str(),data_len);
       buf[data_len-1]=0;
       debug("Reply: '%s' \n",buf+sizeof(UDP::full_header));

       
       // Respond
       debug("<APP SERVER> Sending %li b wrapped in %i b buffer \n",
             response.size(),bufsize);
         
       /** Populate outgoing UDP header */
       UDP::full_header* full_hdr_out = (UDP::full_header*)buf;
       full_hdr_out->udp_hdr.dport = hdr->sport;
       full_hdr_out->udp_hdr.sport = hdr->dport;
       full_hdr_out->udp_hdr.length = __builtin_bswap16(data_len);
       
       /** Populate outgoing IP header */
       full_hdr_out->ip_hdr.saddr = full_hdr->ip_hdr.daddr;
       full_hdr_out->ip_hdr.daddr = full_hdr->ip_hdr.saddr;
       full_hdr_out->ip_hdr.protocol = IP4::IP4_UDP;
       
       /*auto pckt_out = std::make_shared<Packet>
         (Packet(buf,bufsize,Packet::DOWNSTREAM));*/
       
       net->udp_send(pckt_out);
      
          
      return 0;
    });
  
  cout << "<APP SERVER> Listening to UDP port 8080 " << endl;
  
  // Hook up to I/O events and do something useful ...
  
  cout << "Service out! " << endl; 
}
