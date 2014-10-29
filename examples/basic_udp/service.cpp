//#define NDEBUG // Debug supression

#include <os>
#include <iostream>
#include <net/inet>

using namespace std;
using namespace net;

uint8_t* buf = 0;
int bufsize = 0;
uint8_t* prev_data = 0;


void Service::start()
{


  cout << "*** Service is up - with OS Included! ***" << endl;    
  
  auto& mac = Dev::eth(0).mac();
  IP_stack::ifconfig(net::ETH0,{192,168,mac.part[4],mac.part[5]},{255,255,0,0});
  
  /** Trying to access non-existing nic will cause a panic */
  //auto& mac1 = Dev::eth(1).mac();
  //IP_stack::ifconfig(net::ETH1,{192,168,mac1.part[4],mac1.part[5]},{255,255,0,0});
  
  shared_ptr<IP_stack> net(IP_stack::up());
  

  cout << "...Starting UDP server on IP " 
       << net->ip4(net::ETH0).str()
       << endl;

    
  //A one-way UDP server (a primitive test)
  net->udp_listen(8080,[net](uint8_t* const data,int len){
      
      UDP::full_header* full_hdr = (UDP::full_header*)data;
      UDP::udp_header* hdr = &full_hdr->udp_hdr;

      int data_len = __builtin_bswap16(hdr->length) - sizeof(UDP::udp_header);
      auto data_loc = data + sizeof(UDP::full_header);
            
      debug("<APP SERVER> Got %i b of data (%i b frame) from %s:%i -> %s:%i\n",
            data_len, len, full_hdr->ip_hdr.saddr.str().c_str(), 
            __builtin_bswap16(hdr->sport),
            full_hdr->ip_hdr.daddr.str().c_str(), 
            __builtin_bswap16(hdr->dport));
      
      
      for (int i = 0; i < data_len; i++)
        printf("%c", data_loc[i]);
      
      // Craft response
      string response("You said: '"+
                      string((const char*)data_loc,data_len)+
                      "' \n");
      bufsize = response.size() + sizeof(UDP::full_header);
      
      // Ethernet padding if necessary
      if (bufsize < Ethernet::minimum_payload)
        bufsize = Ethernet::minimum_payload;
      
      
      if(buf)
        delete[] buf;
      
      buf = new uint8_t[bufsize]; 
      strcpy((char*)buf + sizeof(UDP::full_header),response.c_str());
      
      
      // Respond
      debug("<APP SERVER> Sending %li b wrapped in %i b buffer \n",
            response.size(),bufsize);
      
      net->udp_send(full_hdr->ip_hdr.daddr, hdr->dport, 
                   full_hdr->ip_hdr.saddr, hdr->sport, buf, bufsize);
      
          
      return 0;
    });
  
  cout << "<APP SERVER> Listening to UDP port 8080 " << endl;  
  
  cout << "Service out! " << endl; 
}
