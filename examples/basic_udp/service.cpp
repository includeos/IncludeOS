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

//#define NDEBUG // Debug supression

#include <os>
#include <iostream>
#include <net/inet>

using namespace std;
using namespace net;

uint8_t* buf = 0;
int bufsize = 0;
uint8_t* prev_data = 0;

class global {
  static int i;
public:
  global(){
    printf("GLOBAL CONSTRUCTOR IN SERVICE %i \n",++i);
  }
  
  void test(){
    printf("I have %i instances \n",i);
  }
  
  int instances(){ return i; }
};

int global::i = 0;

global glob1;

int _test_glob2 = 1;

__attribute__ ((constructor)) void foo(void)
{
  printf("foo is running and printf is available at this point\n");
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



void Service::start()
{



  assert(_test_glob2 == 1);
  
  cout << "*** Service is up - with OS Included! ***" << endl;    
  //global glob2;
  //global glob3;
  glob1.test();
  assert(glob1.instances() == 1);
  
  auto& mac = Dev::eth(0).mac();
  Inet::ifconfig(net::ETH0,{10,0,mac.part[4],mac.part[5]},{255,255,0,0});
  
  /** Trying to access non-existing nic will cause a panic */
  //auto& mac1 = Dev::eth(1).mac();
  //Inet::ifconfig(net::ETH1,{192,168,mac1.part[4],mac1.part[5]},{255,255,0,0});
  
  //Inet* net 
  shared_ptr<Inet> net(Inet::up());
  

  cout << "...Starting UDP server on IP " 
       << net->ip4(net::ETH0).str()
       << endl;

    
  //A one-way UDP server (a primitive test)
  net->udp_listen(8080,[net](uint8_t* const data,int len){
      
      UDP::full_header* full_hdr = (UDP::full_header*)data;
      UDP::udp_header* hdr = &full_hdr->udp_hdr;

      int data_len = __builtin_bswap16(hdr->length) - sizeof(UDP::udp_header);
      auto data_loc = data + sizeof(UDP::full_header);
      
      // Netcat doesn't necessariliy zero-pad the string in UDP
      // ... But this buffer is const
      // auto data_end = data + hdr->length - sizeof(UDP::udp_header);
      // *data_end = 0; 
      
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
  
  // Hook up to I/O events and do something useful ...
  
  cout << "Service out! " << endl; 
}
