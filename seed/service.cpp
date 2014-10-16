#define NDEBUG
#include <os>
#include <iostream>

using namespace std;

void Service::start()
{
  cout << "*** Service is up - with OS Included! ***" << endl;
    
  //IP_stack& net = Dev::eth(0).ip_stack();
  auto& net = Dev::eth(0).ip_stack();
  /** @note: "auto net" would cause copy-construction (!) 
      since auto drops reference, const and volatile qualifiers. */
    
  //A one-way UDP server (a primitive test)
  net.udp_listen(8080,[](uint8_t* const data,int len){
      
      UDP::header* hdr = (UDP::header*)data;      
      
      // stringify (might mess up the ethernet trailer; oh well)
      int data_len = __builtin_bswap16(hdr->length) - 8;
      auto data_loc = data + sizeof(UDP::header);
      auto data_end = data + sizeof(UDP::header) + data_len;
      *data_end = 0; 
      
      debug("<APP SERVER> Got %i b of data! (%i b frame) \n",data_len,len);
      printf("%s",data_loc);
      
      // Free the buffer
      free(data);
      return 0;
    });
  
  cout << "Listening to UDP port 8080 " << endl;
  
  // Hook up to I/O events and do something useful ...
  
  cout << "Service out! " << endl; 
}
